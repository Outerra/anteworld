
#ifdef _WIN32

#define TOKEN_SUPPORT_WSTRING
#include "token.h"
#include "str.h"

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

COID_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
uint charstr::append_wchar_buf_ansi( const wchar_t* src, uints nchars )
{
    uints n = nchars==UMAXS
        ? wcslen((const wchar_t*)src)
        : nchars;

    return WideCharToMultiByte( CP_ACP, 0, src, (int)n, get_append_buf(n), (int)n, 0, 0 );
}

///////////////////////////////////////////////////////////////////////////////
bool token::codepage_to_wstring_append( uint cp, std::wstring& dst ) const
{
    uints i = dst.length();
    dst.resize(i+len());

    wchar_t* data = const_cast<wchar_t*>(dst.data());
    uint n = MultiByteToWideChar( cp, 0, ptr(), (uint)len(), data+i, (uint)len() );

    dst.resize(i+n);
    return n>0;
}

///////////////////////////////////////////////////////////////////////////////
bool token::utf8_to_wchar_buf( wchar_t* dst, uints maxlen ) const
{
    if(maxlen) --maxlen;
    const char* p = ptr();
    bool succ = true;

    uints i=0;
    for(; i<maxlen; ++i)
    {
        if( (uchar)*p <= 0x7f ) {
            dst[i] = *p++;
        }
        else
        {
            uints ne = get_utf8_seq_expected_bytes(p);
            if(p+ne > _pte) {
                //error in input
                succ = false;
                break;
            }

            dst[i] = (wchar_t)read_utf8_seq(p);
            p += ne;
        }
    }

    dst[i] = 0;
    return succ;
}

COID_NAMESPACE_END


#endif //_WIN32

////////////////////////////////////////////////////////////////////////////////

#include "str.h"
#include "pthreadx.h"
#include "atomic/stack_base.h"

///////////////////////////////////////////////////////////////////////////////

#include <sstream>

namespace std {

ostream& operator << (ostream& ost, const coid::charstr& str)
{
    ost.write(str.c_str(), str.len());
    return ost;
}

ostream& operator << (ostream& ost, const coid::token& str)
{
    ost.write(str.ptr(), str.len());
    return ost;
}

} //namespace std

///////////////////////////////////////////////////////////////////////////////

COID_NAMESPACE_BEGIN


struct zstring::zpool
{
    atomic::stack_base<charstr*> _pool;
    uints _maxsize;

    zpool() : _maxsize(SIZE_MAX)
    {}

    void free( charstr* str ) {
        //keep only strings under the max size
        if(str && str->reserved() < _maxsize)
            _pool.push(str);
        else
            delete str;
    }

    charstr* alloc() {
        charstr* str = 0;
        if(!_pool.pop(str))
            str = new charstr;
        return str;
    }

    void set_max_size( uints maxsize ) {
        _maxsize = maxsize;
    }

    static void destroy(void* p) { delete static_cast<zstring::zpool*>(p); }
    static void* create() { return new zstring::zpool; }
};

zstring::zpool* zstring::global_pool()
{
    LOCAL_SINGLETON(zstring::zpool) _pool;
    return _pool.get();
}

zstring::zpool* zstring::thread_local_pool()
{
    static thread_key _TK;

    zstring::zpool* p = (zstring::zpool*)_TK.get();
    if(p)
        return p;
        
    p = new zstring::zpool;
    _TK.set(p);

    return p;
}

zstring::zpool* zstring::local_pool()
{
    return (zstring::zpool*)singleton_register_instance(
        &zstring::zpool::create,
        &zstring::zpool::destroy,
        0,
        0, 0, 0, true);
}

uints zstring::max_size_in_pool( zpool* pool, uints maxsize )
{
    uints old = pool->_maxsize;
    pool->_maxsize = maxsize;
    return old;
}


static const char* nullstring = "";
/*
////////////////////////////////////////////////////////////////////////////////
static atomic::stack_base<charstr*>& zeroterm_pool()
{
    static thread_key _TK;

    pool_t* p = (pool_t*)_TK.get();
    if(p)
        return *p;
        
    p = new pool_t;
    _TK.set(p);

    return *p;
}*/

////////////////////////////////////////////////////////////////////////////////
zstring::~zstring()
{
    free_string();
}

////////////////////////////////////////////////////////////////////////////////
void zstring::free_string()
{
    if(_buf) {
        _pool->free(_buf);
        _buf = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////
zstring::zstring(const zstring& s)
    : _buf(0)
    , _pool(s._pool)
{
    if(s._buf) {
        _zptr = s._buf->ptr();
        _zend = s._buf->ptre();
        //_zptr = _zend = nullstring;
        //get_str() = *s._buf;
    }
    else {
        _zptr = s._zptr;
        _zend = s._zend;
    }
}

////////////////////////////////////////////////////////////////////////////////
zstring::zstring()
    : _zptr(nullstring), _zend(nullstring), _buf(0)
    , _pool(0)
{}

////////////////////////////////////////////////////////////////////////////////
zstring::zstring( zpool* pool )
    : _zptr(nullstring), _zend(nullstring)
    , _buf(0), _pool(pool)
{}

////////////////////////////////////////////////////////////////////////////////
zstring::zstring( const char* sz )
    : _zptr(sz?sz:nullstring), _zend(0), _buf(0), _pool(0)
{}

////////////////////////////////////////////////////////////////////////////////
zstring::zstring(const token& tok)
    : _buf(0)
    , _pool(0)
{
    if(tok.len() == 0)
        _zptr = _zend = nullstring;
    else {
        _zptr = tok.ptr();
        _zend = tok.ptre() - 1;
    }

    get_str();
}

////////////////////////////////////////////////////////////////////////////////
zstring::zstring(const charstr& str)
    : _buf(0)
    , _pool(0)
{
    if(str.len() == 0)
        _zptr = _zend = nullstring;
    else {
        _zptr = str.ptr();
        _zend = str.ptre();
    }

    get_str();
}

////////////////////////////////////////////////////////////////////////////////
zstring& zstring::operator = (const char* sz)
{
    free_string();
    new(this) zstring(sz);

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
zstring& zstring::operator = (const token& tok)
{
    free_string();
    new(this) zstring(tok);

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
zstring& zstring::operator = (const charstr& str)
{
    free_string();
    new(this) zstring(str);

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
zstring& zstring::operator = (const zstring& s)
{
    get_str() = s.get_token();
    _zptr = _zend = nullstring;

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
zstring& zstring::operator << (const char* sz)
{
    if (!_zptr && !_buf)
        new(this) zstring(sz);
    else
        get_str() << sz;

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
zstring& zstring::operator << (const token& tok)
{
    if (!_zptr && !_buf)
        new(this) zstring(tok);
    else
        get_str() << tok;

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
zstring& zstring::operator << (const charstr& str)
{
    if (!_zptr && !_buf)
        new(this) zstring(str);
    else
        get_str() << str;

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
zstring& zstring::operator << (const zstring& s)
{
    if (!_zptr && !_buf)
        new(this) zstring(s);
    else
        get_str() << s.get_token();

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
zstring::operator zstring::unspecified_bool_type () const {
    bool empty = _buf
        ? _buf->is_empty()
        : (_zend ? _zend==_zptr : _zptr[0]==0);
    return empty ? 0 : &zstring::_zptr;
}

////////////////////////////////////////////////////////////////////////////////
const char* zstring::ptr() const
{
    return _buf
        ? _buf->ptr()
        : _zptr;
}

////////////////////////////////////////////////////////////////////////////////
uints zstring::len() const
{
    return _buf
        ? _buf->len()
        : (_zend ? _zend - _zptr : ::strlen(_zptr));
}

////////////////////////////////////////////////////////////////////////////////
const char* zstring::c_str() const
{
    if(_buf)
        return _buf->c_str();
    if(_zend == 0)
        return _zptr;
    
    return *_zend
        ? const_cast<zstring*>(this)->get_str().c_str()
        : _zptr;
}

////////////////////////////////////////////////////////////////////////////////
token zstring::get_token() const
{
    if(_buf)
        return token(*_buf);
    else if(!_zend)
        _zend = _zptr + ::strlen(_zptr);

    return token(_zptr, *_zend ? _zend+1 : _zend);
}

////////////////////////////////////////////////////////////////////////////////
///Get modifiable string
charstr& zstring::get_str( zpool* pool )
{
    if(!_buf) {
        if(pool)
            _pool = pool;
        if(!_pool)
            _pool = thread_local_pool();

        _buf = _pool->alloc();
        _buf->reset();

        if(_zend)
            _buf->set_from_range(_zptr, *_zend ? _zend+1 : _zend);
        else
            _buf->set(_zptr);

        _zptr = _zend = nullstring;
    }

    return *_buf;
}


COID_NAMESPACE_END
