
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
 * PosAm.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Brano Kemen
 * Ladislav Hrabcak
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

#include "logwriter.h"
//#include "../atomic/pool.h"
#include "../atomic/pool_base.h"

#include "../binstream/filestream.h"
#include "../binstream/stdstream.h"

#include "../interface.h"

using namespace coid;

#ifdef SYSTYPE_WIN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static bool enable_debug_out = false;

static void write_console_text( const logmsg& msg )
{
    const charstr& text = msg.str();
    ELogType type = msg.get_type();

    static HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);

    if(type != ELogType::Info) {
        uint flg;

        switch(type) {
        case ELogType::Exception: flg = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        case ELogType::Error:     flg = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
        case ELogType::Warning:   flg = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
        case ELogType::Info:      flg = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        case ELogType::Highlight: flg = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        case ELogType::Debug:     flg = FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        case ELogType::Perf:      flg = FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
        default:                  flg = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
        }

        SetConsoleTextAttribute(hstdout, flg);
    }

    fwrite(text.ptr(), 1, text.len(), stdout);

    if(type != ELogType::Info)
        SetConsoleTextAttribute(hstdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

    if (enable_debug_out)
        stdoutstream::debug_out(text.c_str());
}

#else

static void write_console_text( const logmsg& msg )
{
    fwrite(msg.str().ptr(), 1, msg.str().len(), stdout);
}

#endif

////////////////////////////////////////////////////////////////////////////////

namespace coid {

ref<logmsg> canlog(ELogType type, const tokenhash & hash, const void * inst)
{
    return interface_register::canlog(type, hash, inst);
}

////////////////////////////////////////////////////////////////////////////////

class policy_msg : public policy_base
{
public:
    COIDNEWDELETE("policy_msg");

    typedef pool<policy_msg*> pool_type;

protected:

    pool_type* _pool;
    logmsg* _obj;

protected:

    ///
    explicit policy_msg(logmsg* const obj, pool_type* const p=0) 
        : _pool(p)
        , _obj(obj)
    {}

public:

    logmsg* get() const { return _obj; }

    virtual void _destroy() override
    { 
        DASSERT(_pool != 0);
        
        if(_obj->_logger) {
            //first destroy just queues the message
            _obj->finalize(this);
        }
        else {
            //back to the pool
            policy_msg* t = this;
            _pool->release_instance(t);
        }
    }

    ///
    static policy_msg* create() 
    {
        pool_type& pool = pool_singleton();
        policy_msg* p=0;

        bool make = !pool.create_instance(p);
        if(make)
            p = new policy_msg(new logmsg, &pool);
        else
            p->get()->reset();

        return p;
    }

    static pool_type& pool_singleton() {
        LOCAL_PROCWIDE_SINGLETON_DEF(pool_type) pool;
        return *pool;
    }
};


////////////////////////////////////////////////////////////////////////////////
class logger_file
{
    bofstream _logfile;
    charstr _logbuf;
    charstr _logpath;
    bool _stdout;


    bool check_file_open()
    {
        if(_logfile.is_open() || !_logpath)
            return _logfile.is_open();

        opcd e = _logfile.open(_logpath);
        if(!e) {
            _logfile.xwrite_token_raw(_logbuf);
            _logbuf.free();
        }

        return e==0;
    }

public:
    explicit logger_file( bool std_out ) : _stdout(std_out) {}
    logger_file( const token& path, bool std ) : _logpath(path), _stdout(std)
    {}

    ///Open physical log file. @note Only notes the file name, the file is opened with the next log msg because of potential MT clashes
    void open( charstr filename, bool std ) {
        std::swap(_logpath, filename);
        _stdout = std;
    }

    void write_to_file( const logmsg& lm )
    {
        if(check_file_open())
            _logfile.xwrite_token_raw(lm.str());
        else
            _logbuf << lm.str();

        if(_stdout)
            write_console_text(lm);
    }
};

} //namespace coid

////////////////////////////////////////////////////////////////////////////////
logmsg::logmsg()
    : _logger(0), _type(ELogType::None)
{
    //reserve memory from process allocator
    _str.reserve(128, PROCWIDE_SINGLETON(comm_array_mspace).msp);
}

////////////////////////////////////////////////////////////////////////////////
void logmsg::write()
{
    if(!_str.ends_with('\n'))
        _str.append('\n');

    if(_logger_file)
        _logger_file->write_to_file(*this);
    else
        write_console_text(*this);
}

////////////////////////////////////////////////////////////////////////////////
void logmsg::finalize( policy_msg* p )
{
    if(_type == ELogType::None)
        _type = deduce_type();

    _logger_file = _logger->file();
    _logger->enqueue(ref<logmsg>(p));
    _logger = 0;
}




////////////////////////////////////////////////////////////////////////////////
logger::logger( bool std_out, bool cache_msgs )
	: _stdout(std_out)
{
	SINGLETON(log_writer);

    if (cache_msgs)
        _logfile = ref<logger_file>(new logger_file(std_out));
}

////////////////////////////////////////////////////////////////////////////////
void logger::terminate()
{
    SINGLETON(log_writer).terminate();
}

////////////////////////////////////////////////////////////////////////////////
ref<logmsg> logger::create_msg( ELogType type, const tokenhash& hash, const void* inst, const int64* mstime )
{
    //TODO check hash, inst

    ref<logmsg> rmsg = operator()(type, mstime);
    if(hash)
        rmsg->str() << '[' << hash << "] ";

    return rmsg;
}

////////////////////////////////////////////////////////////////////////////////
ref<logmsg> logger::operator()( ELogType t, const int64* time_ms )
{
    ref<logmsg> msg = create_msg(t);
    charstr& str = msg->str();

    if(time_ms) {
        //str.append_fixed(*time_ms * 1e-3, 9, -3, coid::ALIGN_NUM_RIGHT_FILL_ZEROS);
        str.append_time_formatted(*time_ms, true, 2);
        str.append(' ');
    }

    str << logmsg::type2tok(t);

    return msg;
}

////////////////////////////////////////////////////////////////////////////////
ref<logmsg> logger::create_msg( ELogType type )
{
    ref<logmsg> msg = ref<logmsg>(policy_msg::create());
    msg->set_type(type);
    msg->set_logger(this);

    return msg;
}

////////////////////////////////////////////////////////////////////////////////
void logger::enqueue( ref<logmsg>&& msg )
{
    SINGLETON(log_writer).addmsg(std::forward<ref<logmsg>>(msg));
}

////////////////////////////////////////////////////////////////////////////////
void logger::post( const token& txt, const token& prefix )
{
    token msg = txt;
    ELogType type = logmsg::consume_type(msg);

    ref<logmsg> rmsg = ref<logmsg>(policy_msg::create());
    rmsg->set_logger(this);
    rmsg->set_type(type);
    
    charstr& str = rmsg->str();
    str = logmsg::type2tok(type);
    
    if(prefix)
        str << '[' << prefix << "] ";
    str << msg;

    //enqueue(rmsg);
}

////////////////////////////////////////////////////////////////////////////////
void logger::open(const token& filename)
{
    if (!_logfile)
        _logfile = ref<logger_file>(new logger_file(_stdout));

    _logfile->open(filename, _stdout);
}

////////////////////////////////////////////////////////////////////////////////
void logger::flush()
{
    int maxloop = 3000 / 20;
    while(!SINGLETON(log_writer).is_empty() && maxloop-- > 0)
        sysMilliSecondSleep(20);
}

////////////////////////////////////////////////////////////////////////////////
log_writer::log_writer() 
	: _thread()
	, _queue()
{
    //make sure the dependent singleton gets created
    policy_msg::pool_singleton();
	//policy_pooled<logmsg>::default_pool();

	_thread.create( thread_run_fn, this, 0, "log_writer" );
}

////////////////////////////////////////////////////////////////////////////////
log_writer::~log_writer()
{
    terminate();
}

////////////////////////////////////////////////////////////////////////////////
void log_writer::terminate()
{
    flush();
    _thread.cancel_and_wait(10000);
}

////////////////////////////////////////////////////////////////////////////////
void* log_writer::thread_run()
{
	while (!coid::thread::self_should_cancel()) {
        flush();

#ifdef _DEBUG
		coid::sysMilliSecondSleep(1);
#else
        coid::sysMilliSecondSleep(500);
#endif
	}

    flush();

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
void log_writer::flush()
{
	ref<logmsg> m;

	//int maxloop = 3000 / 20;

	while( _queue.pop(m) ) {
        DASSERT( m->str() );
		m->write();
		m.release();
	}
}
