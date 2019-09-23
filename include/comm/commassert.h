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

#ifndef __COID_COMM_ASSERT__HEADER_FILE__
#define __COID_COMM_ASSERT__HEADER_FILE__

#include "namespace.h"
#include "retcodes.h"

/** \file assert.h
    This header defines various assert macros. The assert macros normally log
    the failed assertion to the assert.log file and throw exception
    ersEXCEPTION afterwards.

    DASSERT*    debug-build only assertions
    RASSERT*    release and debug assertions

    *ASSERTX    provide additional text that will be logged upon failed assertion
    *ASSERTE[X] provide custom exception value that will be thrown upon failed assertion
    *ASSERTL[X] do not throw exceptions at all, only logs the failed assertion
    ASSERT_RET  assert in debug build, log in release, on failed assertion causes return from function where used
*/

////////////////////////////////////////////////////////////////////////////////
COID_NAMESPACE_BEGIN

struct enter_single_thread {
    enter_single_thread(volatile uint& tid);
    ~enter_single_thread();

private:

    volatile uint& _tid;
};

COID_NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////
#ifdef SYSTYPE_MSVC
#define XASSERT                     if(__assert_e) __debugbreak()
#else
#include <cassert>
#define XASSERT                     assert(__assert_e);
#endif

#ifdef _DEBUG
#define XDASSERT XASSERT
#else
#define XDASSERT
#endif

#define XASSERTE(expr)              do{ if(expr) break;  bool __assert_e =



//@{ Runtime assertions
#define RASSERT(expr)               XASSERTE(expr) coid::__rassert(0,__FILE__,__LINE__,__FUNCTION__,#expr); XASSERT; } while(0)
#define RASSERTX(expr,txt)          XASSERTE(expr) coid::__rassert(coid::opt_string() << txt,__FILE__,__LINE__,__FUNCTION__,#expr); XASSERT; } while(0)
//@}

////////////////////////////////////////////////////////////////////////////////

//define COID_ASSERT_LOG if debug asserts should log asserts in release

#if defined(_DEBUG) || defined(COID_DASSERT_LOG)

//@{ Debug-only assertions, release build doesn't execute the expression
#define DASSERT(expr)               XASSERTE(expr) coid::__rassert(0,__FILE__,__LINE__,__FUNCTION__,#expr); XDASSERT; } while(0)
#define DASSERTX(expr,txt)          XASSERTE(expr) coid::__rassert(coid::opt_string() << txt,__FILE__,__LINE__,__FUNCTION__,#expr); XDASSERT; } while(0)
#define DASSERT_ONCE(expr)          do{ static bool once = false; if(expr || once) break;  bool __assert_e = coid::__rassert(0,__FILE__,__LINE__,__FUNCTION__,#expr); once = true; XDASSERT; } while(0)
//@}

//@{ Assert in debug, return \a ret on failed assertion (also in release)
#define DASSERT_RET(expr,ret)       XASSERTE(expr) coid::__rassert(0,__FILE__,__LINE__,__FUNCTION__,#expr); XDASSERT; return ret; } while(0)
#define DASSERT_RETVOID(expr)       XASSERTE(expr) coid::__rassert(0,__FILE__,__LINE__,__FUNCTION__,#expr); XDASSERT; return; } while(0)
//@}

#else

#define DASSERT(expr)
#define DASSERTX(expr,txt)
#define DASSERT_ONCE(expr)

#define DASSERT_RET(expr,ret)       do{ if(expr) break; return ret; } while(0)
#define DASSERT_RETVOID(expr)       do{ if(expr) break; return; } while(0)

#endif //_DEBUG


//@{ Debug assert that won't be switched to logging in release even with COID_DASSERT_LOG defined
#ifdef _DEBUG
#define DASSERTN(expr)              XASSERTE(expr) coid::__rassert(0,__FILE__,__LINE__,__FUNCTION__,#expr); XDASSERT; } while(0)
#else
#define DASSERTN(expr)
#endif


//@{ Assert in debug, log in release, return \a ret on failed assertion (also in release)
#define LASSERT(expr)               XASSERTE(expr) coid::__rassert(0,__FILE__,__LINE__,__FUNCTION__,#expr); XDASSERT; } while(0)
#define LASSERTX(expr,txt)          XASSERTE(expr) coid::__rassert(coid::opt_string() << txt,__FILE__,__LINE__,__FUNCTION__,#expr); XDASSERT; } while(0)
#define LASSERT_RET(expr,ret)       XASSERTE(expr) coid::__rassert(0,__FILE__,__LINE__,__FUNCTION__,#expr); XDASSERT; return ret; } while(0)
#define LASSERT_RETVOID(expr)       XASSERTE(expr) coid::__rassert(0,__FILE__,__LINE__,__FUNCTION__,#expr); XDASSERT; return; } while(0)
#define LASSERT_RETVOIDX(expr,txt)  XASSERTE(expr) coid::__rassert(coid::opt_string() << txt,__FILE__,__LINE__,__FUNCTION__,#expr); XDASSERT; return; } while(0)
#define LASSERT_RETX(expr,ret,txt)  XASSERTE(expr) coid::__rassert(coid::opt_string() << txt,__FILE__,__LINE__,__FUNCTION__,#expr); XDASSERT; return ret; } while(0)
//@}


////////////////////////////////////////////////////////////////////////////////
COID_NAMESPACE_BEGIN

class opt_string;
bool __rassert( const opt_string& txt, const char* file, int line, const char* function, const char* expr );

///Downcast value of integral type, asserting on overflow and underflow
//@return cast value
template <class T, class S>
inline T down_cast(S v) {
    const T vmin = std::numeric_limits<T>::min();
    const T vmax = std::numeric_limits<T>::max();

    DASSERT(v >= vmin && v <= vmax);
    return T(v);
}

struct opcd;
struct token;
class zstring;

///Optional log string
class opt_string
{
public:

    opt_string() : _zstr(0)
    {}

    opt_string(nullptr_t) : _zstr(0)
    {}

    ~opt_string();

    opt_string& operator << (const char* sz);
    
    opt_string& operator << (const token& tok);
    opt_string& operator << (char c);

    template<class Enum>
    opt_string& operator << (typename std::enable_if<std::is_enum<Enum>::value>::type v) {
        return (*this << (typename resolve_enum<Enum>::type)v);
    }

#ifdef SYSTYPE_WIN
# ifdef SYSTYPE_32
    opt_string& operator << (ints i);
    opt_string& operator << (uints i);
# else //SYSTYPE_64
    opt_string& operator << (int i);
    opt_string& operator << (uint i);
# endif
#elif defined(SYSTYPE_32)
    opt_string& operator << (long i);
    opt_string& operator << (ulong i);
#endif //SYSTYPE_WIN

    opt_string& operator << (float d);
    opt_string& operator << (double d);

    zstring* get() const {
        return _zstr;
    }

private:

    zstring* _zstr;
};

COID_NAMESPACE_END

#endif  //!__COID_COMM_ASSERT__HEADER_FILE__
