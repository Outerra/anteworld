#include "net_ul.h"
#include "profiler/profiler.h"
#include "taskmaster.h"

COID_NAMESPACE_BEGIN

const taskmaster::signal_handle taskmaster::invalid_signal = taskmaster::signal_handle(taskmaster::signal_handle::invalid); 

taskmaster::taskmaster(uint nthreads, uint nlowprio_threads)
    : _qsize(0)
    , _hqsize(0)
    , _quitting(false)
    , _nlowprio_threads(nlowprio_threads)
{
    _taskdata.reserve_virtual(8192 * 16);
    _signal_pool.resize(4096);
    _free_signals.reserve(4096);
    for (uint i = 0; i < _signal_pool.size(); ++i) {
        _signal_pool[i].version = 0;
        _free_signals.push(signal_handle(i));
    }

    _threads.alloc(nthreads);
    _threads.for_each([&](threadinfo& ti, uints id) {
        ti.order = uint(id);
        ti.master = this;
        ti.tid.create(threadfunc, &ti, 0, "taskmaster");
        });
}

taskmaster::~taskmaster() {
    terminate(false);
}

void taskmaster::wait() {
    CPU_PROFILE_SCOPE_COLOR("taskmaster::wait", 0x80, 0, 0);
    std::unique_lock<std::mutex> lock(_sync);
    if (get_order() < _nlowprio_threads) {
        while (!_qsize) // handle spurious wake-ups
            _cv.wait(lock);
    }
    else {
        while (!_hqsize) // handle spurious wake-ups
            _hcv.wait(lock);
    }
}

void taskmaster::run_task(invoker_base* task)
{
    CPU_PROFILE_FUNCTION();
    uints id = _taskdata.get_item_id((granule*)task);
    //coidlog_devdbg("taskmaster", "thread " << order << " processing task id " << id);

#ifdef _DEBUG
    thread::set_name("<unknown task>"_T);
#endif

    DASSERT_RET(_taskdata.is_valid_id(id));
    task->invoke();

#ifdef _DEBUG
    thread::set_name("<no task>"_T);
#endif

    const signal_handle handle = task->signal();
    if (handle.is_valid()) {
        std::unique_lock<std::mutex> lock(_signal_sync);
        signal& s = _signal_pool[handle.index()];
        --s.ref;
        if (s.ref == 0) {
            s.version = (s.version + 1) % 0xffFF;
            signal_handle free_handle = signal_handle::make(s.version, handle.index());
            _free_signals.push(free_handle);
        }
    }

    _taskdata.del_range((granule*)task, align_to_chunks(task->size(), sizeof(granule)));
}

void* taskmaster::threadfunc( int order )
{
    get_order() = order;

    thread::set_affinity_mask((uint64)1 << order);
    coidlog_info("taskmaster", "thread " << order << " running");
    char tmp[64];
    sprintf_s(tmp, "taskmaster %d", order);
    profiler::set_thread_name(tmp);

    wait();
    while (!_quitting) {
        // TODO there are 3 locks here: wait, _ready_jobs.pop and lock(_sync), they can be merged into one
        invoker_base* task = 0;
        for (int prio = 0; prio < (int)EPriority::COUNT; ++prio) {
            const bool can_run = prio != (int)EPriority::LOW || order < _nlowprio_threads || order == -1;
            if (can_run && _ready_jobs[prio].pop(task)) {
                {
                    std::unique_lock<std::mutex> lock(_sync);
                    --_qsize;
                    if (prio != (int)EPriority::LOW) --_hqsize;
                }
                run_task(task);
                break;
            }
        }
        wait();
    }

    coidlog_info("taskmaster", "thread " << order << " exiting");

    return 0;
}


bool taskmaster::is_signaled(signal_handle handle, bool lock)
{
    DASSERT_RET(handle.is_valid(), false);

    signal& s = _signal_pool[handle.index()];

    if (lock) _signal_sync.lock();
    const uint version = s.version;
    const uint ref = s.ref;
    if (lock) _signal_sync.unlock();

    return version != handle.version() || ref == 0;
}

taskmaster::signal_handle taskmaster::alloc_signal()
{
    signal_handle handle;
    if (!_free_signals.pop(handle)) return invalid_signal;

    signal& s = _signal_pool[handle.index()];
    s.ref = 1;

    return handle;
}

void taskmaster::increment(signal_handle* handle)
{
    std::unique_lock<std::mutex> lock(_signal_sync);
    if (!handle) return;

    if (handle->is_valid()) {
        signal& s = _signal_pool[handle->index()];
        if (is_signaled(*handle, false)) {
            *handle = alloc_signal();
        }
        else {
            ++s.ref;
        }
    }
    else {
        *handle = alloc_signal();
    }
}

void taskmaster::notify_all() {
    {
        std::unique_lock<std::mutex> lock(_sync);
        _qsize += (int)_threads.size();
        _hqsize += (int)_threads.size();
    }
    _cv.notify_all();
    _hcv.notify_all();
}

void taskmaster::enter_critical_section(critical_section& critical_section, int spin_count)
{
    for (;;) {
        for (int i = 0; i < spin_count; ++i) {
            if (critical_section.value == 0 && atomic::cas(&critical_section.value, 1, 0) == 0) {
                return;
            }
        }

        invoker_base* task = 0;
        const int order = get_order();
        for (int prio = 0; prio < (int)EPriority::COUNT; ++prio) {
            const bool can_run = prio != (int)EPriority::LOW || (order < _nlowprio_threads && order != -1);
            if (can_run && _ready_jobs[prio].pop(task)) {
                {
                    std::unique_lock<std::mutex> lock(_sync);
                    --_qsize;
                    if (prio != (int)EPriority::LOW) --_hqsize;
                }
                run_task(task);
                break;
            }
        }
        if (!task)
            thread::wait(0);
    }
}

void taskmaster::leave_critical_section(critical_section& critical_section)
{
    const int32 prev = atomic::cas(&critical_section.value, 0, 1);
    DASSERTN(prev == 1);
}


void taskmaster::wait(signal_handle signal)
{
    if (!signal.is_valid()) return;

    const int order = get_order();
    while (!is_signaled(signal, true)) {
        invoker_base* task = 0;
        for (int prio = 0; prio < (int)EPriority::COUNT; ++prio) {
            const bool can_run = prio != (int)EPriority::LOW || (order < _nlowprio_threads&& order != -1);
            if (can_run && _ready_jobs[prio].pop(task)) {
                {
                    std::unique_lock<std::mutex> lock(_sync);
                    --_qsize;
                    if (prio != (int)EPriority::LOW) --_hqsize;
                }
                run_task(task);
                break;
            }
        }
        if (!task)
            thread::wait(0);
    }
}

void taskmaster::terminate(bool empty_queue)
{
    if (empty_queue) {
        while (_qsize > 0)
            thread::wait(0);
    }

    _quitting = true;

    notify_all();

    //wait for cancellation
    _threads.for_each([](threadinfo& ti) {
        thread::join(ti.tid);
        });
}

taskmaster::signal_handle taskmaster::create_signal()
{
    std::unique_lock<std::mutex> lock(_signal_sync);

    signal_handle handle;
    if (!_free_signals.pop(handle)) return invalid_signal;

    signal& s = _signal_pool[handle.index()];
    s.ref = 1;

    return handle;
}

void taskmaster::trigger_signal(signal_handle handle)
{
    std::unique_lock<std::mutex> lock(_signal_sync);

    signal& s = _signal_pool[handle.index()];
    --s.ref;
    if (s.ref == 0) {
        s.version = (s.version + 1) % 0xffFF;
        signal_handle free_handle = signal_handle::make(s.version, handle.index());
        _free_signals.push(free_handle);
    }
}


COID_NAMESPACE_END
