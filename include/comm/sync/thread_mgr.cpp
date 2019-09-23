
#include "thread_mgr.h"
#include "../log/logger.h"

#ifdef SYSTYPE_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>

typedef HRESULT(__stdcall *SetThreadDescription_t)(
    HANDLE hThread,
    PCWSTR lpThreadDescription
    );

const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

static void set_thread_name(coid::thread_manager::info* ti)
{
    static SetThreadDescription_t set_thread_desc = 0;
    static bool check = true;
    if (check) {
        HMODULE handle = LoadLibraryA("Kernel32.dll");
        set_thread_desc = (SetThreadDescription_t)GetProcAddress(handle, "SetThreadDescription");

        check = false;
    }

    //use new Win10 method if available
    if (set_thread_desc) {
        wchar_t namebuf[256];
        coid::token(ti->name).utf8_to_wchar_buf(namebuf, 256);

        if (SUCCEEDED(set_thread_desc(ti->handle, namebuf)))
            return;
    }

#ifndef SYSTYPE_MINGW
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = ti->name.c_str();
    info.dwThreadID = ti->tid;
    info.dwFlags = 0;

    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)& info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
#endif
}

#else

static void set_thread_name(coid::thread_manager::info* ti) {}

#endif //SYSTYPE_WIN


namespace coid {

////////////////////////////////////////////////////////////////////////////////
void thread_manager::thread_name(thread_t tid, const token& name)
{
    GUARDME;
    info* const* pti = _hash.find_value(tid);
    if (pti && (*pti)->name != name) {
        (*pti)->name = name;
        set_thread_name(*pti);
    }
}

////////////////////////////////////////////////////////////////////////////////
thread thread_manager::thread_start(thread_manager::info* ti)
{
    thread_t tid;

#ifdef SYSTYPE_WIN
    ti->handle = (void*)CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread_manager::def_thread, ti, 0, (ulong*)& tid);
#else
    pthread_create(&tid, 0, thread_manager::def_thread, ti);
#endif

    ti->tid = tid;
    return tid;
}

////////////////////////////////////////////////////////////////////////////////
void* thread_manager::def_thread(void* pinfo)
{
    info* ti = (info*)pinfo;

#ifdef SYSTYPE_WIN
    if (!ti->name.is_empty())
        set_thread_name(ti);
#endif

    //wait until the spawner inserts the id
    while (ti->tid == thread::invalid())
        sysMilliSecondSleep(0);

    ti->mgr->thread_register(ti);

    //invoke begin callback
    if (ti->mgr->_cbk_begin)
        ti->mgr->_cbk_begin();


    //we're not using pthread's cancellation
    //pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, 0 );
    //pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, 0 );

    void* res = 0;
    try {
        res = ti->entry();
    }
    catch (thread::CancelException&) {
    }
    catch (const std::bad_alloc& e) {
        memtrack_dump("memory.log", false);
        auto log = canlog(log::exception, "threadmgr");
        if (log)
            log->str() << "out of memory in thread " << ti->name << ": " << e.what() << '\r';
        throw;
    }
    catch (const std::exception& e) {
        auto log = canlog(log::exception, "threadmgr");
        if (log)
            log->str() << "exception in thread " << ti->name << ": " << e.what() << '\r';
        throw;
    }
    /*catch(...) {
        DASSERT(false && "unknown exception thrown!");
        res = 0;
    }*/

    //invoke end callback
    if (ti->mgr->_cbk_end)
        ti->mgr->_cbk_end();

    ti->mgr->thread_unregister(ti->tid);
    delete ti;

    return res;
}

////////////////////////////////////////////////////////////////////////////////
void thread_manager::set_begin_end_callbacks(thread_beginend_callback begin, thread_beginend_callback end)
{
    _cbk_begin = begin;
    _cbk_end = end;
}

} //namespace coid
