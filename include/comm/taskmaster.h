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
//#include "sync/mutex.h"
//#include "sync/guard.h"
#include "pthreadx.h"
#include "log/logger.h"
#include <mutex>
#include <condition_variable>

COID_NAMESPACE_BEGIN

/**
    Taskmaster runs a set of worker threads and a queue of tasks that are processed by the worker threads.
    It's intended for short tasks that can be parallelized across the preallocated number of working
    threads, but it also supports long-duration jobs that can run on a limited number of worker threads
    and have a lower priority.

    Additionally, jobs can be synchronized or unsynchronized.

    - long duration threads will prioritize short jobs if available
    - thread waiting for completion of given set of jobs will also partake in processing of the same
      type of jobs
    - there can be multiple threads that wait for completion of jobs
    - if there's a thread waiting for job completion, all worker threads prioritize given job type
    - if there are multiple completion requests at different synchronization levels, the one with
      the highest priority (lowest sync level number) is prioritized over the other

**/
class taskmaster
{
public:

    //@param nthreads total number of job threads to spawn
    //@param nlong_threads number of long-duration job threads (<= nthreads)
    taskmaster( uint nthreads, uint nlong_threads )
        : _nlong_threads(nlong_threads)
        , _qsize(0)
        , _quitting(false)
    {
        _taskdata.reserve(8192);
        _synclevels.reserve(16, false);

        _threads.alloc(nthreads);
        _threads.for_each([&](threadinfo& ti) {
            ti.order = uint(&ti - _threads.ptr());
            ti.master = this;
            ti.tid.create(threadfunc, &ti, 0, "taskmaster");
        });
    }

    ///Set number of threads that can process long-duration jobs
    //@param nlong_threads number of long-duration job threads
    void set_long_duration_threads( uint nlong_threads ) {
        _nlong_threads = nlong_threads;
    }

    ///Add synchronization level
    //@param level sync level number (0 highest priority for completion)
    //@param name sync level name
    //@return level
    uint add_task_level( uint level, const token& name ) {
        sync_level& sl = _synclevels.get_or_add(level);
        sl.name = name;
        sl.njobs = 0;

        return level;
    }

    ///Push task (function and its arguments) into queue for processing by worker threads
    //@param tlevel task level to push into (<0 unsynced long duration tasks, >=0 registered task level
    //@param fn function to run
    //@param args arguments needed to invoke the function
    template <typename Fn, typename ...Args>
    void push( int tlevel, const Fn& fn, Args&& ...args )
    {
        using callfn = invoker<Fn, Args...>;

        //lock to access allocator and semaphore
        std::unique_lock<std::mutex> lock(_sync);

        granule* p = alloc_data(sizeof(callfn));
        auto task = new(p) callfn(tlevel, fn, std::forward<Args>(args)...);

        push_to_queue(task);
    }

    ///Push task (function and its arguments) into queue for processing by worker threads
    //@param tlevel task level to push into (<0 unsynced long duration tasks, >=0 registered task level
    //@param fn member function to run
    //@param obj object reference to run the member function on. Can be a pointer or a smart ptr type which resolves to the object with * operator
    //@param args arguments needed to invoke the function
    template <typename Fn, typename C, typename ...Args>
    void push_memberfn( int tlevel, Fn fn, const C& obj, Args&& ...args )
    {
        static_assert(std::is_member_function_pointer<Fn>::value, "fn must be a function that can be invoked as ((*obj).*fn)(args)");

        using callfn = invoker_memberfn<Fn, C, Args...>;

        //lock to access allocator and semaphore
        std::unique_lock<std::mutex> lock(_sync);

        granule* p = alloc_data(sizeof(callfn));
        auto task = new(p) callfn(tlevel, fn, obj, std::forward<Args>(args)...);

        push_to_queue(task);
    }


    ///Wait for completion of all jobs of given task level
    //@param tlevel task level to wait for
    //@note this waiting thread can participate in job completion, and all worker threads will prioritize
    // jobs of requested task level unless there's a higher-priority wait in another thread
    void wait( int tlevel )
    {
        if (tlevel >= (int)_synclevels.size())
            return;

        //signal to worker threads there's a priority task level
        auto& level = _synclevels[tlevel];
        ++level.priority;

        while (level.njobs > 0) {
            process_specific_task(tlevel);
        }

        --level.priority;
    }

    ///Terminate all task threads
    //@param empty_queue if true, wait until the task queue empties, false finish only currently processed tasks
    void terminate(bool empty_queue)
    {
        if (empty_queue) {
            while (_qsize > 0)
                thread::wait(0);
        }

        _quitting = true;

        notify((int)_threads.size());

        //wait for cancellation
        _threads.for_each([](threadinfo& ti) {
            thread::join(ti.tid);
        });
    }

protected:

    ///
    struct threadinfo
    {
        thread tid;
        taskmaster* master;

        int order;


        threadinfo() : master(0), order(-1)
        {}

        bool is_long_duration_thread() const {
            return order < master->_nlong_threads;
        }
    };

    ///
    struct sync_level
    {
        std::atomic_int njobs;          //< number of jobs on this sync level
        std::atomic_int priority;

        charstr name;

        sync_level()
            : njobs(0)
            , priority(0)
        {}
    };

    ///Unit of allocation for tasks
    struct granule
    {
        uint64 dummy[2 * sizeof(uints) / 4];
    };

    granule* alloc_data(uints size)
    {
        auto base = _taskdata.get_array().ptr();

        uints n = align_to_chunks(size, sizeof(granule));
        granule* p = _taskdata.add_range_uninit(n);

        //no rebase
        DASSERT(base == _taskdata.get_array().ptr());

        coidlog_devdbg("taskmaster", "pushed task id " << _taskdata.get_item_id(p));
        return p;
    }

    ///
    struct invoker_base {
        virtual void invoke() = 0;
        virtual size_t size() const = 0;

        invoker_base(int sync)
            : _sync(sync)
        {}

        int task_level() const {
            return _sync;
        }

    protected:
        int _sync;
    };

    template <typename Fn, typename ...Args>
    struct invoker_common : invoker_base
    {
        invoker_common(int sync, const Fn& fn, Args&& ...args)
            : invoker_base(sync)
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
            ((*obj).*_fn)(std::get<I>(_tuple)...);
        }

    private:

        typedef std::tuple<Args...> tuple_t;

        Fn _fn;
        tuple_t _tuple;
    };

    ///invoker for lambdas and functions
    template <typename Fn, typename ...Args>
    struct invoker : invoker_common<Fn, Args...>
    {
        invoker(int sync, const Fn& fn, Args&& ...args)
            : invoker_common(sync, fn, std::forward<Args>(args)...)
        {}

        void invoke() override final {
            invoke_fn(make_index_sequence<sizeof...(Args)>());
        }

        size_t size() const override final {
            return sizeof(*this);
        }
    };

    ///invoker for member functions
    template <typename Fn, typename C, typename ...Args>
    struct invoker_memberfn : invoker_common<Fn, Args...>
    {
        invoker_memberfn(int sync, Fn fn, const C& obj, Args&&... args)
            : invoker_common(sync, fn, std::forward<Args>(args)...)
            , _obj(obj)
        {}

        void invoke() override final {
            invoke_memberfn(_obj, make_index_sequence<sizeof...(Args)>());
        }

        size_t size() const override final {
            return sizeof(*this);
        }

    private:

        C _obj;
    };


    void push_to_queue(invoker_base* task)
    {
        int sync = task->task_level();

        if (sync >= 0) {
            DASSERT(sync < (int)_synclevels.size());

            if (sync < (int)_synclevels.size())
                ++_synclevels[sync].njobs;
        }

        _queue.push(task);
        
        ++_qsize;
        _cv.notify_one();
    }

private:

    taskmaster(const taskmaster&);

    static void* threadfunc(void* data) {
        threadinfo* ti = (threadinfo*)data;
        return ti->master->threadfunc(ti->order);
    }

    void* threadfunc( int order )
    {
        do
        {
            wait();

            //could have been woken up for termination
            if (_quitting)
                break;

            //if a wait is active, all worker threads should prioritize given task level
            int priority_level = get_priority_level();
            bool longthread = order < _nlong_threads;

            invoker_base* task = 0;
            RASSERT(_queue.pop(task));

            //if priority level is set, process only tasks of that level
            //short-duration threads should process only short tasks

            if ((priority_level < 0 && (longthread || task->task_level() >= 0))
                || task->task_level() == priority_level)
            {
                run_task(task, order);
            }
            else {
                _queue.push(task);
                notify();
            }
        }
        while (1);

        return 0;
    }

    int get_priority_level() const {
        const sync_level* sl = _synclevels.find_if([](const sync_level& sl) {
            return sl.priority > 0;
        });

        return sl ? int(sl - _synclevels.ptr()) : -1;
    }

    bool process_specific_task( int tlevel )
    {
        //run through the queue once to pick tasks from given level
        int qsize = _qsize;
        sync_level& sl = _synclevels[tlevel];
        invoker_base* task;

        while (qsize-->0 && sl.njobs > 0 && try_wait()) {
            RASSERT(_queue.pop(task));

            if (task->task_level() == tlevel) {
                run_task(task, -1);
                return true;
            }

            //push back
            _queue.push(task);
            notify();
        }

        return false;
    }

    void run_task( invoker_base* task, int order )
    {
        uints id = _taskdata.get_item_id((granule*)task);
        coidlog_devdbg("taskmaster", "thread " << order << " processing task id " << id);

        DASSERT(_taskdata.is_valid_id(id));
        task->invoke();

        int sync = task->task_level();
        if (sync >= 0)
            --_synclevels[sync].njobs;

        _taskdata.del_range((granule*)task, align_to_chunks(task->size(), sizeof(granule)));
    }

    void notify() {
        {
            std::unique_lock<std::mutex> lock(_sync);
            ++_qsize;
        }
        _cv.notify_one();
    }

    void notify( int n ) {
        {
            std::unique_lock<std::mutex> lock(_sync);
            _qsize += n;
        }
        _cv.notify_all();
    }

    void wait() {
        std::unique_lock<std::mutex> lock(_sync);
        while(!_qsize) // handle spurious wake-ups
            _cv.wait(lock);
        --_qsize;
    }

    bool try_wait() {
        std::unique_lock<std::mutex> lock(_sync);
        if(_qsize) {
            --_qsize;
            return true;
        }
        return false;
    }

private:

    std::mutex _sync;
    std::condition_variable _cv;
    volatile int _qsize;                //< current queue size, used also as a semaphore
    volatile bool _quitting;

    slotalloc<granule> _taskdata;

    queue<invoker_base*> _queue;

    dynarray<threadinfo> _threads;
    volatile int _nlong_threads;

    dynarray<sync_level> _synclevels;
};

COID_NAMESPACE_END
