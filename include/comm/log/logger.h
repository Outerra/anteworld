
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
 * Ladislav Hrabcak
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
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

#ifndef __COMM_LOGGER_H__
#define __COMM_LOGGER_H__

#include "../str.h"
#include "../ref.h"
#include "../function.h"
#include "../alloc/slotalloc.h"

COID_NAMESPACE_BEGIN

namespace log {

enum type {
    none = -1,
    exception = 0,
    error,
    warning,
    highlight,
    info,
    debug,
    perf,
    last,
};

static const type* values() {
    static type _values[] = {
        none,
        exception,
        error,
        warning,
        highlight,
        info,
        debug,
        perf,
        last,
    };
    return _values;
}

static const char** names() {
    static const char* _names[] = {
        "none",
        "exception",
        "error",
        "warning",
        "highlight",
        "info",
        "debug",
        "perf",
        "last",
        0
    };
    return _names;
}

static const char* name(type t) {
    return t >= none && t <= last
        ? names()[t + 1]
        : 0;
}

} //namespace log

class logger_file;
class logger;
class logmsg;
class policy_msg;

//@return logmsg object if given log type and source is currently allowed to log
ref<logmsg> canlog( log::type type, const tokenhash& hash = tokenhash(), const void* inst = 0 );


#ifdef COID_VARIADIC_TEMPLATES

///Formatted log message
//@param type log level
//@param hash source identifier (used for filtering)
//@param fmt @see charstr.print
template<class ...Vs>
void printlog( log::type type, const tokenhash& hash, const token& fmt, Vs&&... vs);

#endif //COID_VARIADIC_TEMPLATES

////////////////////////////////////////////////////////////////////////////////
//@{ Log message with specified severity
#define coidlog_none(src, msg)    do{ ref<coid::logmsg> q = coid::canlog(coid::log::none, src ); if(q) {q->str() << msg; }} while(0)
#define coidlog_debug(src, msg)   do{ ref<coid::logmsg> q = coid::canlog(coid::log::debug, src ); if(q) {q->str() << msg; }} while(0)
#define coidlog_perf(src, msg)    do{ ref<coid::logmsg> q = coid::canlog(coid::log::perf, src ); if(q) {q->str() << msg; }} while(0)
#define coidlog_info(src, msg)    do{ ref<coid::logmsg> q = coid::canlog(coid::log::info, src ); if(q) {q->str() << msg; }} while(0)
#define coidlog_msg(src, msg)     do{ ref<coid::logmsg> q = coid::canlog(coid::log::highlight, src ); if(q) {q->str() << msg; }} while(0)
#define coidlog_warning(src, msg) do{ ref<coid::logmsg> q = coid::canlog(coid::log::warning, src ); if(q) {q->str() << msg; }} while(0)
#define coidlog_error(src, msg)   do{ ref<coid::logmsg> q = coid::canlog(coid::log::error, src ); if(q) {q->str() << msg; }} while(0)
//@}

///Create a perf object that logs the time while the scope exists
#define coidlog_perf_scope(src, msg) \
   ref<coid::logmsg> perf##line = coid::canlog(coid::log::perf, src); if(perf##line) perf##line->str() << msg

///Log fatal error and throw exception with the same message
#define coidlog_exception(src, msg)     do{ ref<coid::logmsg> q = coid::canlog(coid::log::exception, src ); if(q) {q->str() << msg; throw coid::exception() << msg; }} while(0)

//@{ Log error if condition fails
#define coidlog_assert(test, src, msg)          do { if(!(test)) coidlog_error(src, msg); } while(0)
#define coidlog_assert_ret(test, ret, src, msg) do { if(!(test)) { coidlog_error(src, msg); return ret; } } while(0)
#define coidlog_assert_retvoid(test, src, msg)  do { if(!(test)) { coidlog_error(src, msg); return; } } while(0)
//@}

///Debug message existing only in debug builds
#ifdef _DEBUG
#define coidlog_devdbg(src, msg)  do{ ref<coid::logmsg> q = coid::canlog(coid::log::debug, src ); if(q) {q->str() << msg; }} while(0)
#else
#define coidlog_devdbg(src, msg)
#endif

////////////////////////////////////////////////////////////////////////////////

/*
 * Log message object, returned by the logger.
 * Message is written in policy_log::release() upon calling destructor of ref
 * with policy policy_log.
 */
class logmsg
    //: public policy_pooled<logmsg>
{
protected:

    friend class policy_msg;

    logger* _logger = 0;
    ref<logger_file> _logger_file;

    tokenhash _hash;
    log::type _type = log::none;
    charstr _str;
    uint64 _time = 0;

public:

    COIDNEWDELETE(logmsg);

    logmsg();

    logmsg( logmsg&& other )
        : _type(other._type)
    {
        _logger = other._logger;
        other._logger = 0;

        _logger_file.takeover(other._logger_file);
        _str.takeover(other._str);
    }

    void set_logger( logger* l ) { _logger = l; }

    void reset() {
        _str.reset();
        _logger = 0;
        _logger_file.release();
    }

    void write();

    ///Consume type prefix from the message
    static log::type consume_type( token& msg )
    {
        log::type t = log::info;
        if(msg.consume_icase("error:"))
            t = log::error;
        else if(msg.consume_icase("warn:") || msg.consume_icase("warning:"))
            t = log::warning;
        else if(msg.consume_icase("info:"))
            t = log::info;
        else if(msg.consume_icase("msg:"))
            t = log::highlight;
        else if(msg.consume_icase("dbg:") || msg.consume_icase("debug:"))
            t = log::debug;
        else if(msg.consume_icase("perf:"))
            t = log::perf;

        msg.skip_whitespace();
        return t;
    }

    static const token& type2tok( log::type t )
    {
        static token st[1 + int(log::last)]={
            "",
            "FATAL: ",
            "ERROR: ",
            "WARNING: ",
            "INFO: ",
            "INFO: ",
            "DEBUG: ",
            "PERF: ",
        };
        static token empty;

        return t<log::last ? st[1 + int(t)] : empty;
    }

    log::type deduce_type() const {
        token tok = _str;
        return consume_type(tok);
    }

    log::type get_type() const { return _type; }

    void set_type(log::type t) { _type = t; }

    void set_time(uint64 ns) {
        _time = ns;
    }

    int64 get_time() const { return _time; }

    void set_hash(const tokenhash& hash) { _hash = hash; }
    const tokenhash& get_hash() { return _hash; }

    charstr& str() { return _str; }
    const charstr& str() const { return _str; }

protected:

    //@return true if looger should be flushed (msg ended with \r)
    bool finalize( policy_msg* p );
};

typedef ref<logmsg> logmsg_ptr;

////////////////////////////////////////////////////////////////////////////////

#ifdef COID_VARIADIC_TEMPLATES

///Formatted log message
//@param type log level
//@param hash source identifier (used for filtering)
//@param fmt @see charstr.print
template<class ...Vs>
inline void printlog( log::type type, const tokenhash& hash, const token& fmt, Vs&&... vs)
{
    ref<logmsg> msgr = canlog(type, hash);
    if (!msgr)
        return;

    charstr& str = msgr->str();
    str.print(fmt, std::forward<Vs>(vs)...);
}

#endif //COID_VARIADIC_TEMPLATES

struct log_filter {
    typedef function<void(ref<logmsg>&)> filter_fun;
    filter_fun _filter_fun;
    charstr _module;
    uint _log_level;

    log_filter(const filter_fun& fn, const token& module, uint level)
    : _filter_fun(fn)
    , _module(module)
    , _log_level(level)
    {}
};


////////////////////////////////////////////////////////////////////////////////
/*
 * USAGE :
 *
 * class fwmlogger : public logger
 * {
 * public:
 *     fwmlogger() : logger("fwm.log") {}
 * };
 * #define fwmlog(msg) (*SINGLETON(fwmlogger)()<<msg)
 *
 * USAGE:
 *
 * fwmlog("ugh" << x << "asd")
 */
class logger
{
protected:
    slotalloc<log_filter> _filters;
    ref<logger_file> _logfile;

    log::type _minlevel;
    bool _stdout;

public:

    //@param std_out true if messages should be printed to stdout as well
    //@param cache_msgs true if messages should be cached until the log file is specified with open()
    logger( bool std_out, bool cache_msgs );
    virtual ~logger() {}

    static void terminate();

    void open( const token& filename );

    void post( const token& msg, const token& prefix = token() );

#ifdef COID_VARIADIC_TEMPLATES

    ///Formatted log message
    template<class ...Vs>
    void print( const token& fmt, Vs&&... vs ) {
        ref<logmsg> msgr = create_msg(log::none, tokenhash());
        if(!msgr)
            return;

        charstr& str = msgr->str();
        str.print(fmt, std::forward<Vs>(vs)...);
    }

    ///Formatted log message
    template<class ...Vs>
    void print( log::type type, const tokenhash& hash, const void* inst, const token& fmt, Vs&&... vs )
    {
        ref<logmsg> msgr = create_msg(type, hash, inst);
        if(!msgr)
            return;

        charstr& str = msgr->str();
        str.print(fmt, std::forward<Vs>(vs)...);
    }

#endif

    //@return logmsg, filling the prefix by the log type (e.g. ERROR: )
    ref<logmsg> operator()( log::type type = log::info, const tokenhash& hash = "", const int64* time_ms = 0 );

    //@return an empty logmsg object
    ref<logmsg> create_msg( log::type type, const tokenhash& hash );

    ///Creates logmsg object if given log message type is enabled
    //@param type log level
    //@param hash tokenhash identifying the client (interface) name
    //@param inst optional instance id
    //@return logmsg reference or null if not enabled
    ref<logmsg> create_msg( log::type type, const tokenhash& hash, const void* inst, const int64* mstime = 0 );

    const ref<logger_file>& file() const { return _logfile; }

    virtual void enqueue( ref<logmsg>&& msg );

    void flush();

    void set_log_level( log::type minlevel = log::last );

    static void enable_debug_out(bool en);

    uints register_filter(const log_filter& filter) { _filters.push(filter); return _filters.count() - 1; }
    void unregister_filter(uint pos) { _filters.del_item(pos); }
};


////////////////////////////////////////////////////////////////////////////////
///
class stdoutlogger : public logger
{
public:

    stdoutlogger() : logger(true, false)
    {}
};


COID_NAMESPACE_END

#endif // __COMM_LOGGER_H__
