#pragma once

/* ***** BEGIN LICENSE BLOCK *****
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Mozilla Public License Version
* 1.1 (the "License"); you may not use this file except in compliance with
* the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is COID/comm module.
*
* The Initial Developer of the Original Code is
* Outerra.
* Portions created by the Initial Developer are Copyright (C) 2017
* the Initial Developer. All Rights Reserved.
*
* Contributor(s):
* Brano Kemen
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the MPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the MPL, the GPL or the LGPL.
*
* ***** END LICENSE BLOCK ***** */

#include "namespace.h"

#include "trait.h"
#include "alloc/slotalloc.h"
#include "bitrange.h"
#include "sync/queue.h"
#include "pthreadx.h"
#include "log/logger.h"
#include <mutex>
#include <condition_variable>

COID_NAMESPACE_BEGIN

/**
    Taskmaster runs a set of worker threads and a queue of tasks that are processed by the worker threads.

    Tasks with higher priorities are processed before tasks with lower priorities. LOW priority tasks can
    run on a limited number of worker threads. Each job can be associated with a signal and user can wait
    for this signal. Several jobs can share single signal, but single job can not trigger multiple signals.

    When a thread is waiting for a signal it processes other tasks in queue.

    Basic usage:
        coid::taskmaster::signal_handle signal;
        for (int i = 0; i < 10; ++i) {
            taskmaster->push(coid::taskmaster::EPriority::NORMAL, &signal, [i](){ foo(i); });
        }
        taskmaster->wait(signal);
**/
class taskmaster
{
public:
    struct critical_section
    {
        volatile int32 value = 0;
    };

    struct signal_handle
    {
        enum
        {
            invalid = 0xffFFffFF,
            index_mask = 0x0000ffFF,
            version_shift = 16
        };

        static signal_handle make(uint version, uint index) { return signal_handle((version << version_shift) | (index & index_mask)); }

        signal_handle() : value(invalid) {}
        explicit signal_handle(uint32 value) : value(value) {}

        bool is_valid() const { return value != invalid; }
        uint index() const { return value & index_mask; }
        uint version() const { return value >> version_shift; }

        uint32 value = invalid;
    };

    static const signal_handle invalid_signal;

    enum class EPriority {
        HIGH,
        NORMAL,
        LOW,                            //< for limited "long run" threads

        COUNT
    };

    //@param nthreads total number of job threads to spawn
    //@param nlong_threads number of low-prio job threads (<= nthreads)
    taskmaster(uint nthreads, uint nlowprio_threads);

    ~taskmaster();

    uints get_workers_count() const { return _threads.size(); }

    ///Run fn(index) in parallel in task level 0
    //@param first begin index value
    //@param last end index value
    //@param fn function(index) to run
    template <typename Index, typename Fn>
    void parallel_for(Index first, Index last, const Fn& fn) {
        signal_handle signal;
        for (; first != last; ++first) {
            push(EPriority::HIGH, &signal, fn, first);
        }

        wait(signal);
    }

    ///Push task (functor, e.g. lamda) into queue for processing by worker threads
    //@param priority task priority, higher priority tasks are processed before lower priority
    //@param signal signal to trigger when the task finishes
    //@param fn functor to run
    template <typename Fn>
    void push_functor(EPriority priority, signal_handle* signal, const Fn& fn)
    {
        push(priority, signal, [](const Fn& fn) {
            fn();
            }, fn);
    }

    ///Push task (function and its arguments) into queue for processing by worker threads
    //@param priority task priority, higher priority tasks are processed before lower priority
    //@param signal signal to trigger when the task finishes
    //@param fn function to run
    //@param args arguments needed to invoke the function
    template <typename Fn, typename ...Args>
    void push(EPriority priority, signal_handle* signal, const Fn& fn, Args&& ...args)
    {
        using callfn = invoker<Fn, Args...>;

        {
            //lock to access allocator and semaphore
            std::unique_lock<std::mutex> lock(_sync);

            granule* p = alloc_data(sizeof(callfn));
            increment(signal);
            auto task = new(p) callfn(signal ? *signal : invalid_signal, fn, std::forward<Args>(args)...);

            _ready_jobs[(int)priority].push_front(task);

            ++_qsize;
            if (priority != EPriority::LOW) ++_hqsize;
        }
        _cv.notify_one();
        if (priority != EPriority::LOW) {
            _hcv.notify_one();
        }
    }

    ///Push task (function and its arguments) into queue for processing by worker threads
    //@param priority task priority, higher priority tasks are processed before lower priority
    //@param signal signal to trigger when the task finishes
    //@param fn member function to run
    //@param obj object pointer to run the member function on
    //@param args arguments needed to invoke the function
    template <typename Fn, typename C, typename ...Args>
    void push_memberfn(EPriority priority, signal_handle* signal, Fn fn, C* obj, Args&& ...args)
    {
        static_assert(std::is_member_function_pointer<Fn>::value, "fn must be a function that can be invoked as ((*obj).*fn)(args)");

        using callfn = invoker_memberfn<Fn, C*, Args...>;

        {
            //lock to access allocator and semaphore
            std::unique_lock<std::mutex> lock(_sync);

            granule* p = alloc_data(sizeof(callfn));
            increment(signal);
            auto task = new(p) callfn(signal ? *signal : invalid_signal, fn, obj, std::forward<Args>(args)...);

            _ready_jobs[(int)priority].push_front(task);

            ++_qsize;
            if (priority != EPriority::LOW) ++_hqsize;
        }
        _cv.notify_one();
        if (priority != EPriority::LOW) {
            _hcv.notify_one();
        }
    }

    ///Push task (function and its arguments) into queue for processing by worker threads
    //@param priority task priority, higher priority tasks are processed before lower priority
    //@param signal signal to trigger when the task finishes
    //@param fn member function to run
    //@param obj object reference to run the member function on. Can be a smart ptr type which resolves to the object with * operator
    //@param args arguments needed to invoke the function
    template <typename Fn, typename C, typename ...Args>
    void push_memberfn(EPriority priority, signal_handle* signal, Fn fn, const C& obj, Args&& ...args)
    {
        static_assert(std::is_member_function_pointer<Fn>::value, "fn must be a function that can be invoked as ((*obj).*fn)(args)");

        using callfn = invoker_memberfn<Fn, C, Args...>;

        {
            //lock to access allocator and semaphore
            std::unique_lock<std::mutex> lock(_sync);

            granule* p = alloc_data(sizeof(callfn));
            increment(signal);
            auto task = new(p) callfn(signal ? *signal : invalid_signal, fn, obj, std::forward<Args>(args)...);

            _ready_jobs[(int)priority].push_front(task);

            ++_qsize;
            if (priority != EPriority::LOW) ++_hqsize;
        }
        _cv.notify_one();
        if (priority != EPriority::LOW) {
            _hcv.notify_one();
        }
    }

    /// Enter critical section; no two threads can be in the same critical section at the same time
    /// other threads process other tasks while waiting to enter critical section
    //@param spin_count number of spins before trying to process other tasks
    //@note never call enter(A) enter(B) exit(A) exit(B) in that order, since it can cause
    // deadlock thanks to taskmaster's nature
    void enter_critical_section(critical_section& critical_section, int spin_count = 1024);

    /// Leave critical section
    //@note only thread which entered the critical section can leave it
    void leave_critical_section(critical_section& critical_section);

    ///Wait for signal to become signaled
    //@param signal signal to wait for
    //@note each time a task is pushed to queue and has a signal associated, it increments the signal's counter.
    // When the task finishes it decrements the counter. Once the counter == 0, the signal is in signaled state.
    // Multiple tasks can use the same signal.
    void wait(signal_handle signal);

    ///Terminate all task threads
    //@param empty_queue if true, wait until the task queue empties, false finish only currently processed tasks
    void terminate(bool empty_queue);

    ///Create standalone signal not associated with any task. It can be used to wait for any arbitrary stuff.
    //@note The signal's counter is initialized = 1 -> wait(signal) will wait untill counter == 0)
    // use taskmaster::trigger_signal(signal) do decrement the counter
    // if you just want to wait until some task finishes, you do not need to use this, see "Basic usage"
    signal_handle create_signal();

    ///Manually decrements signal's counter. When the counter reaches 0, the signal is in signaled state
    ///and waiting entities can progress further.
    void trigger_signal(signal_handle handle);

protected:

    ///
    struct threadinfo
    {
        thread tid;
        taskmaster* master;

        int order;


        threadinfo() : master(0), order(-1)
        {}
    };

    ///Unit of allocation for tasks
    struct granule
    {
        uint8 dummy[8 * sizeof(void*)];
    };

    static int& get_order()
    {
        static thread_local int order = -1;
        return order;
    }

    granule* alloc_data(uints size)
    {
        uints n = align_to_chunks(size, sizeof(granule));
        granule* p = _taskdata.add_contiguous_range_uninit(n);

        //coidlog_devdbg("taskmaster", "pushed task id " << _taskdata.get_item_id(p));
        return p;
    }

    ///
    struct invoker_base {
        virtual void invoke() = 0;
        virtual size_t size() const = 0;

        invoker_base(signal_handle signal)
            : _signal(signal)
            , _tid(thread::self())
        {}

        signal_handle signal() const {
            return _signal;
        }

    protected:
        signal_handle _signal;
        thread_t _tid;
    };

    template <typename Fn, typename ...Args>
    struct invoker_common : invoker_base
    {
        invoker_common(signal_handle signal, const Fn& fn, Args&& ...args)
            : invoker_base(signal)
            , _fn(fn)
            , _tuple(std::forward<Args>(args)...)
        {}

    protected:

        template <size_t ...I>
        void invoke_fn(index_sequence<I...>) {
            _fn(std::get<I>(_tuple)...);
        }

        template <class C, size_t ...I>
        void invoke_memberfn(C& obj, index_sequence<I...>) {
            (obj.*_fn)(std::get<I>(_tuple)...);
        }

    private:

        typedef std::tuple<std::remove_reference_t<Args>...> tuple_t;

        Fn _fn;
        tuple_t _tuple;
    };

    ///invoker for lambdas and functions
    template <typename Fn, typename ...Args>
    struct invoker : invoker_common<Fn, Args...>
    {
        invoker(signal_handle signal, const Fn& fn, Args&& ...args)
            : invoker_common<Fn, Args...>(signal, fn, std::forward<Args>(args)...)
        {}

        void invoke() override final {
            this->invoke_fn(make_index_sequence<sizeof...(Args)>());
        }

        size_t size() const override final {
            return sizeof(*this);
        }
    };

    ///invoker for member functions (on copied objects)
    template <typename Fn, typename C, typename ...Args>
    struct invoker_memberfn : invoker_common<Fn, Args...>
    {
        invoker_memberfn(signal_handle signal, Fn fn, const C& obj, Args&&... args)
            : invoker_common<Fn, Args...>(signal, fn, std::forward<Args>(args)...)
            , _obj(obj)
        {}

        invoker_memberfn(signal_handle signal, Fn fn, C&& obj, Args&&... args)
            : invoker_common<Fn, Args...>(signal, fn, std::forward<Args>(args)...)
            , _obj(std::forward<C>(obj))
        {}

        void invoke() override final {
            this->invoke_memberfn(_obj, make_index_sequence<sizeof...(Args)>());
        }

        size_t size() const override final {
            return sizeof(*this);
        }

    private:

        C _obj;
    };

    ///invoker for member functions (on pointers)
    template <typename Fn, typename C, typename ...Args>
    struct invoker_memberfn<Fn, C*, Args...> : invoker_common<Fn, Args...>
    {
        invoker_memberfn(signal_handle signal, Fn fn, C* obj, Args&&... args)
            : invoker_common<Fn, Args...>(signal, fn, std::forward<Args>(args)...)
            , _obj(obj)
        {}

        void invoke() override final {
            this->invoke_memberfn(*_obj, make_index_sequence<sizeof...(Args)>());
        }

        size_t size() const override final {
            return sizeof(*this);
        }

    private:

        C* _obj;
    };

    ///invoker for member functions (on irefs)
    template <typename Fn, typename C, typename ...Args>
    struct invoker_memberfn<Fn, iref<C>, Args...> : invoker_common<Fn, Args...>
    {
        invoker_memberfn(signal_handle signal, Fn fn, const iref<C>& obj, Args&&... args)
            : invoker_common<Fn, Args...>(signal, fn, std::forward<Args>(args)...)
            , _obj(obj)
        {}

        void invoke() override final {
            this->invoke_memberfn(*_obj, make_index_sequence<sizeof...(Args)>());
        }

        size_t size() const override final {
            return sizeof(*this);
        }

    private:

        iref<C> _obj;
    };

    struct signal
    {
        volatile int ref;
        uint32 version;
    };

private:

    taskmaster(const taskmaster&);

    static void* threadfunc(void* data) {
        threadinfo* ti = (threadinfo*)data;
        return ti->master->threadfunc(ti->order);
    }

    void* threadfunc(int order);
    void run_task(invoker_base* task);
    bool is_signaled(signal_handle handle, bool lock);
    signal_handle alloc_signal();
    void increment(signal_handle* handle);
    void notify_all();
    void wait();

private:

    std::mutex _sync;
    std::mutex _signal_sync;
    std::condition_variable _cv;        //< for threads which can process low prio tasks
    std::condition_variable _hcv;       //< for threads which can not process low prio tasks
    std::atomic_int _qsize;             //< current queue size, used also as a semaphore
    std::atomic_int _hqsize;            //< current queue size without low prio tasks
    volatile bool _quitting;

    slotalloc_atomic<granule> _taskdata;

    dynarray<threadinfo> _threads;
    volatile int _nlowprio_threads;

    dynarray<signal> _signal_pool;
    dynarray<signal_handle> _free_signals;
    queue<invoker_base*> _ready_jobs[(int)EPriority::COUNT];
};

COID_NAMESPACE_END
