#include "net_ul.h"
#include "profiler/profiler.h"
#include "taskmaster.h"

COID_NAMESPACE_BEGIN

const taskmaster::signal_handle taskmaster::invalid_signal = taskmaster::signal_handle(taskmaster::signal_handle::invalid); 

taskmaster::~taskmaster() {
    terminate(false);
}

void* taskmaster::threadfunc( int order )
{
    uint notify_counter = 0;

    get_order() = order;

    thread::set_affinity_mask((uint64)1 << order);
    coidlog_info("taskmaster", "thread " << order << " running");
    char tmp[64];
    sprintf_s(tmp, "taskmaster %d", order);
    profiler::set_thread_name(tmp);

    do
    {
        wait();
        if (_quitting) break;

        invoker_base* task = 0;
        for(int prio = 0; prio < (int)EPriority::COUNT; ++prio) {
            const bool can_run = prio != (int)EPriority::LOW || order < _nlowprio_threads || order == -1;
            if (can_run && _ready_jobs[prio].pop(task)) {
                notify_counter = 0;
                run_task(task, order);
                break;
            }
        }
        
        if (!task) {
            // we did not pop any task, this might happen if there's a low prio task 
            // and thread can not process it, so let's wake other thread, hopefully one which 
            // can process low prio tasks
            notify();
            ++notify_counter;
            if (notify_counter > _threads.size() * 4) {
                // all low prio threads are most likely busy so we give up our timeslice since there might be 
                // other sleeping threads ready to work (e.g. jobmaster)
                thread::wait(0);
                notify_counter = 0;
            }
        }

    }
    while (!_quitting);

    coidlog_info("taskmaster", "thread " << order << " exiting");

    return 0;
}

COID_NAMESPACE_END
