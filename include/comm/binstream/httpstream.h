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

#ifndef __COID_COMM_HTTPSTREAM2__HEADER_FILE__
#define __COID_COMM_HTTPSTREAM2__HEADER_FILE__

#include "../namespace.h"
#include "../str.h"
#include "../rnd.h"
#include "../dir.h"
#include "../net.h"

#include "cachestream.h"
#include "binstreambuf.h"
#include "filestream.h"
#include "txtstream.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
class httpstream : public binstream
{
public:
    ///Http header
    struct header
    {
        uints _content_length;
        uints _header_length;
        int _errcode;

        timet _if_mdf_since;

        enum { TE_CHUNKED = 0x01, TE_TRAILERS = 0x02, TE_DEFLATE = 0x04, TE_GZIP = 0x08, TE_IDENTITY = 0x10, };
        uint _te;

        bool _bclose;
        bool _bhttp10;
        //bool _isdir;
        //bool _isfile;

        charstr _method;
        charstr _fullpath;

        token _query;               //< query part (after ?)
        token _relpath;             //< relative path

        charstr _set_cookie;
        charstr _location;
        charstr _content_encoding;

        header()
        {
            _errcode = 0;
            _header_length = 0;
            _content_length = UMAXS;
            _bclose = _bhttp10 = false;
            _te = 0;
        }

        bool is_chunked() const     { return (_te & TE_CHUNKED) != 0; }

        opcd decode( bool is_listener, httpstream& bin, binstream* log );
    };

public:
    ///
    class cachestreamex : public cachestream
    {
        uints _hdrpos;
        uint rdchunk;
        binstreambuf buf;
        charstr _tmp;
        bool final;

        static const token& CLheader()
        { static token _T = "Content-Length: 0                   \r\n\r\n";  return _T; }

    public:

        cachestreamex() {
            _hdrpos = UMAXS;
            _tmp = "Content-Length: ";  //length=16
            rdchunk = 0;
            final=false;
        }


        bool everything_read( uints size ) {
            return size_read() >= size;
        }

        void append_token(const token& tok)
        {
            _cot.add_bin_from((const uchar*)tok.ptr(), tok.len());
        }

        opcd chunked_read_raw( void* p, uints& len )
        {
            opcd e;
            uints olen = len;

            if(final)
                return ersNO_MORE;

            if( rdchunk == 0 ) {
                do {
                    buf.reset_write();
                    e = read_until( substring::crlf(), &buf );
                    if(e)
                        return e;
                } while( buf.is_empty() );

                token chunk = buf;
                rdchunk = chunk.touint_base_and_shift(16);

                if( rdchunk>0 )
                    e = ersRETRY;
                else {
                    char crlf[2];
                    uints crlf_len=2;
                    read_raw(crlf, crlf_len);

                    final = true;
                    e = ersNO_MORE;
                }
            }
            else if( rdchunk < len ) {
                uints tlen = rdchunk;
                e = read_raw( p, tlen );

                len = len - rdchunk + tlen;
                if(!e)  e = ersRETRY;
            }
            else
                e = read_raw( p, len );

            rdchunk -= uint(olen-len);

            return e;
        }

        opcd set_hdr_pos( uint64 content_len )
        {
            if( content_len == UMAX64 ) {
                _hdrpos = len() + 16;   //skip "Content-Length: "

                return write_token_raw( CLheader() );
            }
            else if( content_len == 0 ) {
                _hdrpos = 0;

                return write_token_raw("\r\n");
            }
            else {
                _hdrpos = 0;

                _tmp.resize(16);
                _tmp << content_len << "\r\n\r\n";

                return write_token_raw(_tmp);
            }
        }

        opcd on_cache_flush( void* p, uints size, bool final )
        {
            //if(!final)
            //    return ersDENIED;   //no multiple packets supported

            DASSERT( !final || _hdrpos != UMAXS );

            if(final && _hdrpos) {
                //write msg len
                char* pc = (char*) get_raw( _hdrpos, 20 );

                //write content length
				if(len() > 0) {
					uints csize = len() - _hdrpos - 20-4;
					charstrconv::num_formatter<uints>::insert( pc, 20, csize, 10, 20, ALIGN_NUM_RIGHT );
				}
            }

            if(final)
                _hdrpos = UMAXS;

            return 0;
        }

        void chunked_acknowledge( bool eat = false )
        {
            if(eat)
                rdchunk = 0;
            else {
                if( rdchunk>0 )
                    throw ersIO_ERROR "data left in input buffer";

                if(!final) {
                    uints len=0;
                    opcd e = chunked_read_raw( 0, len );
                    if( e != ersNO_MORE )
                        throw ersIO_ERROR "data left in input buffer";
                }
            }

            final = false;
            acknowledge(eat);
        }
    };


    static const substring& substring_proto()
    {
        static substring _ss("://", false);
        return _ss;
    }

public:

    ///Called to decode the value of unknown http header
    //@param name header
    //@param value value string
    virtual opcd on_extra_header( const token& name, token value )
    {
        return 0;
    }

    virtual opcd on_new_read()          { return 0; }
    virtual opcd on_new_write()         { return 0; }


    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return fATTR_IO_FORMATTING | fATTR_HANDSHAKING | fATTR_READ_UNTIL;
    }

    virtual opcd write_raw( const void* p, uints& len )
    {
        opcd e = check_write();
        if(e) return e;

        return _cache.write_raw( p, len );
    }

    virtual opcd read_raw( void* p, uints& len )
    {
        opcd e = check_read();
        if(e) return e;

        if(_hdr->is_chunked())
            return _cache.chunked_read_raw(p, len);

        if( _hdr->_content_length != UMAXS
            &&  _cache.everything_read(_hdr->_header_length + _hdr->_content_length) )
            return ersNO_MORE;

        return _cache.read_raw( p, len );
    }

    virtual void flush()
    {
        uints written = _cache.len();
        check_write(written);

        _cache.flush();
        _flags &= ~fWSTATUS;

        if( (_flags & fLISTENER)  &&  _hdr->_bclose )
            _cache.close(true);
    }

    virtual void acknowledge( bool eat=false )
    {
        check_read();

        if( _hdr->is_chunked() )
            _cache.chunked_acknowledge(eat);
        else
            _cache.acknowledge(eat);

        _flags &= ~fRSTATUS;

        if( !(_flags & fLISTENER)  &&  _hdr->_bclose )
        {
            _cache.close(true);
        }
    }

    virtual bool is_open() const
    {
        return _cache.is_open();
    }

    virtual void reset_read()
    {
        _flags &= ~fRSTATUS;
        _cache.reset_read();
    }

    virtual void reset_write()
    {
        _flags &= ~fWSTATUS;
        _cache.reset_write();
    }

    opcd consume_body()
    {
        uints len = _hdr->_content_length;
        if(_hdr->is_chunked()) {
            uint8 buf[256];
            opcd e;
            do {
                uints len=256;
                e = _cache.chunked_read_raw(buf, len);
            }
            while(e==0 || e==ersRETRY);
            return e;
        }
        else
            return _cache.read_raw_scrap(len);
    }

    opcd send_error( const token& errstr )
    {
        set_content_type( "text/plain" );
        opcd e = new_write(0);
        if(e) return e;

        flush();
        return 0;
    }

    ///Send file as a reply or query
    /**
        @return
        ersUNAVAILABLE if another/previous write wasn't flushed still
        ersNOT_FOUND if file wasn't found
        ersINVALID_TYPE if the file isn't regular
        ersIGNORE if the file wasn't modified since the time specified in _hdr->_if_mdf_since
        ersDENIED if the file could not be opened for reading
        or other opcd errors related to the transport stream

        @note with formdata==true, content-type will be "multipart/form-data; boundary=_mime_"
         and @a mime should contain the boundary string separator
    **/
    opcd send_file( const charstr& file, const token& mime, bool formdata=false, const token& filename=token() )
    {
        if( (_flags & fWSTATUS) != 0 )
            return ersUNAVAILABLE;  //another write in progress

        directory::xstat st;
        if( !directory::stat(file,&st) )
            return ersNOT_FOUND;

        if( !directory::is_regular(st.st_mode) )
            return ersINVALID_TYPE;

        if( (_flags & fLISTENER)
            && _hdr->_if_mdf_since!=0
            && st.st_mtime <= _hdr->_if_mdf_since )
        {   return ersIGNORE; }

        bifstream bif(file);
        if( !bif.is_open() )
            return ersDENIED;

        charstr& ct = get_content_type();
        if(formdata)
            ct << "multipart/form-data; boundary=" << mime << "\r\n";
        else
            ct << mime << "\r\n";

        set_modified( st.st_mtime );

        static const token fd1 = "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"";//test.bin"
        static const token fd2 = "\"\r\nContent-Type: application/octet-stream\r\n\r\n";
        static const token fde = "\r\n--";
        static const token fdh = "--";

        token fn;
        uint64 csize = st.st_size;

        if(formdata) {
            fn = filename ? filename : token(file).cut_right_group_back("\\/");
            csize += fdh.len() + mime.len() + fd1.len() + fn.len() + fd2.len() + fde.len() + mime.len() + fdh.len();
        }

        opcd e = new_write(csize);
        if(e) return e;

        if(formdata) {
            _cache.append_token(fdh);
            _cache.append_token(mime);

            _cache.append_token(fd1);
            _cache.append_token(fn);
            _cache.append_token(fd2);
        }

        //now flush the headers
        _cache.flush_cache(!formdata);

        //and write the content
        bif.transfer_to( *_cache.bound() );

        if(formdata) {
            _cache.append_token(fde);
            _cache.append_token(mime);
            _cache.append_token(fdh);
            _cache.flush_cache(true);
        }

        _flags &= ~fWSTATUS;

        if( (_flags & fLISTENER)  &&  _hdr->_bclose )
            _cache.close(true);

        return 0;
    }


    httpstream()
    {
        _flags = 0;

        _hdrx = new header;
        _hdr = _hdrx;
        _cache.reserve_buffer_size(1024);

        //_cache.bind(net);
        _tcache.bind(_cache);

        set_defaults();
    }

    httpstream( header& hdr, cachestream& cache )
    {
        _flags = fSKIP_HEADER;

        _hdr = &hdr;
        _cache.bind( *cache.bound() );
        cache.swap(_cache);
        _tcache.bind(_cache);

        set_defaults();
    }

    void set_defaults()
    {
        _content_type_qry = "Content-Type: text/plain\r\n";
        _content_type_rsp = "Content-Type: text/plain\r\n";
        _content_type_len = 14;

        _errcode = "200 OK";
    }

    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS )
    {
        return _cache.read_until( ss, bout, max_size );
    }

    virtual opcd peek_read( uint timeout )  { return _cache.peek_read(timeout); }
    virtual opcd peek_write( uint timeout ) { return _cache.peek_write(timeout); }


    virtual opcd bind( binstream& bin, int io=0 )
    {
        return _cache.bind( bin, io );
    }

    virtual opcd set_timeout( uint ms )
    {
        return _cache.set_timeout(ms);
    }

    virtual opcd transfer_to( binstream& dst, uints datasize=UMAXS, uints* size_written=0, uints blocksize = 4096 )
	{
        return _cache.transfer_to(dst, datasize, size_written);
    }



    charstr& set_host( const token& tok )
    {
        token uri = tok;

        if(uri.first_char() == '?') {
            _urihdr.resize(token(_urihdr).count_notchar('?'));
            _urihdr << uri;
        }
        else if(uri.first_char() == '/')
            _urihdr = uri;
        else {
            uri.cut_left( substring_proto(), token::cut_trait_remove_sep_default_empty() );

            token host = uri.cut_left('/', token::cut_trait_keep_sep_with_source());
            _urihdr = uri;

            (_proxyreq = "Host: ") << host << "\r\n";
        }
    
        return _urihdr;
    }
/*
    void set_host( const netAddress& addr )
    {
        charstr a;
		addr.getHost(a, true);
        (_proxyreq = "Host: ") << a << "\r\n";
        _urihdr = a;
    }
*/
    void set_listener( bool is_listener )
    {
        if(is_listener) {
            _flags |= fLISTENER;
            _errcode = "200 OK";
        }
        else
            _flags &= ~fLISTENER;
    }

    ///Set response http error code (also indicates that the writing mode is response)
    void set_response( const token& errcode )
    {
        _flags |= fLISTENER;
        _errcode = errcode;
    }

    ///Indicate that the writing mode is a http request
    void set_request()
    {
        _flags &= ~fLISTENER;
    }


    ///Set content type for request or query
    void set_content_type( const token& ct )
    {
        charstr& dst = get_content_type();
        dst += !ct.is_empty() ? ct : "text/plain";
        dst += "\r\n";
    }

    void set_skip_header()
    {
        _flags |= fSKIP_HEADER;
    }

    ///Set optional headers, returns a reference to charstr that can be manipulated
    charstr& set_optional_header( const token& opt )
    {
        _opthdr = opt;
        return _opthdr;
    }

    ///Get optional headers, returns a reference to charstr that can be manipulated
    charstr& get_optional_header() {
        return _opthdr;
    }

    charstr& get_cookie_header() {
        return _cookie;
    }

    ///Set connection type: Close (true) or Keep-Alive (false)
    void set_connection_type( bool bclose )
    {
        if(bclose)
            _flags |= fCLOSE_CONN;
        else
            _flags &=~(int)fCLOSE_CONN;
    }

    ///Set time for Last-Modified header. It is cleared after every reply.
    void set_modified( time_t mdf )
    {
        tmmodif = mdf;
    }

    bool is_writting()                  { return (_flags & fWSTATUS) != 0; }
    bool is_reading()                   { return (_flags & fRSTATUS) != 0; }

    ///Read header of the reply
    opcd read_header()                  { return check_read(); }

    const header& get_header() const    { return *_hdr; }


    void set_cache_size( uints sizer, uints sizew )   { _cache.reserve_buffer_size( sizer, sizew ); }

    cachestream& get_cache_stream()     { return _cache; }

    binstream& text()                   { check_write(); return _tcache; }


protected:

    charstr& get_content_type() {
        charstr& dst = (_flags & fLISTENER)!=0
            ? _content_type_rsp
            : _content_type_qry;

        dst.resize(_content_type_len);
        return dst;
    }

protected:

    txtstream _tcache;
    cachestreamex _cache;

    local<header>  _hdrx;
    header* _hdr;

    charstr _tmp;

    token _errcode;
    charstr _proxyreq;
    charstr _urihdr;
    charstr _opthdr;
    charstr _cookie;

    charstr _content_type_qry;
    charstr _content_type_rsp;
    uint _content_type_len;

    timet tmmodif;

    netAddress _addr;
    uint _flags;
    enum {
        fRSTATUS                = 0x01,         //< closed/transm.open
        fWSTATUS                = 0x02,         //< reading/all read
        fLISTENER               = 0x04,         //< request/response header mode

        fSKIP_HEADER            = 0x20,
        fCLOSE_CONN             = 0x40,

        REQUEST_NEW             = 0xba,         //< new connection request
        REQUEST_OLD             = 0xc9,         //< estabilished conn.req.
    };


protected:

    opcd check_read()
    {
        if( (_flags & fRSTATUS) != 0 )
            return 0;
        //receive header
        return new_read();
    }

    opcd check_write( uint64 len=UMAX64 )
    {
        if( (_flags & fWSTATUS) != 0 )
            return 0;
        //write header
        return new_write(len);
    }

    ///
    opcd new_write( uint64 content_len )
    {
        if( _flags & fWSTATUS )
            return ersUNAVAILABLE;

        //_cache.set_timeout(0);

        static token _RN( "\r\n" );
        static token _POST( "POST " );
        static token _GET( "GET " );
        static token _POST1(
            " HTTP/1.1\r\n"
            );
        static token _POST2(
            "Accept: text/plain\r\n"
            "MIME-Version: 1.0\r\n"
            );

        static token _RESP(
            //"X-PLEASE_WAIT: .\r\n"
            "\r\n"
            "Server: COID/HT\r\n"
            "MIME-Version: 1.0\r\n"
            );

        _flags |= fWSTATUS;
        bool is_listener = (_flags & fLISTENER) != 0;

        if(is_listener)
        {
            _tcache << "HTTP/1.1 " << _errcode << _RESP;
            
            if(content_len)
                _tcache << _content_type_rsp;

            if( tmmodif != 0 ) {
                static token _MDF("Last-Modified: ");

                _tmp.reset();
                _tmp.append_date_gmt( tmmodif );
                _tcache << _MDF << _tmp << _RN;
                tmmodif = 0;
            }
            /*else {
                static token _CCO("Cache-Control: no-cache\r\n");
                _tcache << _CCO;
            }*/
        }
        else {
            _tcache << (content_len ? _POST : _GET)
                << _urihdr << _POST1 << _proxyreq << _POST2;
            
            if(content_len)
                _tcache << _content_type_qry;
        }

        _tmp.reset();
        _tmp.append_date_gmt( timet::current() );
        _tcache << "Date: " << _tmp << _RN;

        static token _AE( "Accept-Encoding: \r\n" );
        _tcache << _AE;

        if(_cookie) {
            _tcache << "Cookie: " << _cookie;
            if(!_cookie.ends_with(_RN))
                _tcache << _RN;
        }

        //
        static token _CONC( "Connection: Close\r\n" );
        static token _CONKA( "Connection: Keep-Alive\r\n" );
        bool close = (is_listener && _hdr->_bclose) || (!is_listener && (_flags & fCLOSE_CONN)!=0);
        _tcache << (close ? _CONC : _CONKA );

        if( !_opthdr.is_empty() ) {
            _tcache << _opthdr;

            if( !_opthdr.ends_with(_RN) )
                _tcache << _RN;
        }

        _cache.set_hdr_pos( content_len );

        return on_new_write();
    }

    ///
    opcd new_read()
    {
        if( _flags & fRSTATUS )
            return ersUNAVAILABLE;

        opcd e;

        if( (_flags & fSKIP_HEADER) == 0 )
        {
            binstreambuf buf;
            e = _hdr->decode( (_flags & fLISTENER)!=0, *this, &buf );

            if(e) {
                 //_cache.set_timeout(10);
                _cache.read_until( substring::zero(), &buf );
                 //_cache.set_timeout(0);

                 /*bofstream bf(  (_flags & fLISTENER)
                     ? "httpstream_req.log?wc+"
                     : "httpstream_resp.log?wc+"
                     );
                 txtstream txt(bf);
                 txt << (token)buf
                     << "-------------------------------------------------------------------------------------\n\n";
				 */
                 return ersFAILED;
            }
        }

        e = on_new_read();
        _flags |= fRSTATUS;

        return e;
    }
};


////////////////////////////////////////////////////////////////////////////////
inline opcd httpstream::header::decode( bool is_listener, httpstream& http, binstream* log )
{
    binstreambuf buf;
    cachestream& bin = http.get_cache_stream();

    _bclose = true;
    _te = 0;
    _header_length = 0;
    _content_length = UMAXS;
    _if_mdf_since = 0;
    _set_cookie.reset();
    _location.reset();
    _content_encoding.reset();

    //skip any nonsense 
    opcd e;
    for(;;)
    {
        e = bin.read_until( substring::crlf(), &buf );
        if(e)
            return e;

        token t = buf;

        if(log)
            *log << t << "\r\n";

        if( is_listener  ||  t.begins_with_icase("http/") ) break;

        buf.reset_write();
    }
     
    token n, tok = buf;
    tok.skip_char(' ');

    if(is_listener)
    {
        token meth = tok.cut_left(' ');
        _method = meth;

        token path = tok.cut_left(' ');

        static token _HTTP = "http://";

        //check the path
        if( path.begins_with_icase(_HTTP) )
        {
            path.shift_start( _HTTP.len() );
            path.skip_notchar('/');
        }

        _fullpath = path;
        path = _fullpath;

        _relpath = path.cut_left('?');
        _query = path;


        token proto = tok.cut_left(' ');
        if( proto.cmpeqi("http/1.0") )
            _bhttp10 = _bclose = true;
        else if( proto.cmpeqi("http/1.1") )
            _bhttp10 = _bclose = false;
        else
            return ersFE_UNRECG_REQUEST "unknown protocol";
    }
    else
    {
        token proto= tok.cut_left(' ');
        if( proto.cmpeqi("http/1.0") )
            _bhttp10 = _bclose = true;
        else if( proto.cmpeqi("http/1.1") )
            _bhttp10 = _bclose = false;
        else
            return ersFE_UNRECG_REQUEST "unknown protocol";

        token errcd = tok.cut_left(' ');
        _errcode = errcd.toint();
    }

    //read remaining headers
    for(;;)
    {
        buf.reset_write();

        e = bin.read_until( substring::crlf(), &buf );
        if(e)
            return e;

        if( buf.is_empty() ) {
            _header_length = bin.size_read();
            return 0;
        }

        token h = buf;
        if(log)
            *log << h << "\r\n";

        token h1 = h.cut_left(':');
        h.skip_char(' ');

        if( h1.cmpeqi("TE")  ||  h1.cmpeqi("Transfer-Encoding") )
        {
            for(;;)
            {
                token k = h.cut_left_group(", ", token::cut_trait_remove_all() );
                if( k.is_empty() )  break;

                if( k.cmpeqi("deflate") )
                    _te |= TE_DEFLATE;
                else if( k.cmpeqi("gzip") )
                    _te |= TE_GZIP;
                else if( k.cmpeqi("chunked") )
                    _te |= TE_CHUNKED;
                else if( k.cmpeqi("trailers") )
                    _te |= TE_TRAILERS;
                else if( k.cmpeqi("identity") )
                    _te |= TE_IDENTITY;
            }
        }
        else if( h1.cmpeqi("Connection") )
        {
            for(;;)
            {
                token k = h.cut_left_group(", ", token::cut_trait_remove_all() );
                if( k.is_empty() )  break;

                if( k.cmpeqi("close") )
                    _bclose = true;
                else if( k.cmpeqi("keep-alive") )
                    _bclose = false;
            }
        }
        else if( h1.cmpeqi("Location") )
        {
            _location = h;
        }
        else if( h1.cmpeqi("Content-Length") )
        {
            _content_length = h.toint();
        }
        else if( h1.cmpeqi("Content-Encoding") )
        {
            _content_encoding = h;
        }
        else if( h1.cmpeqi("If-Modified-Since") || h1.cmpeqi("Last-Modified") )
        {
            h.todate_gmt(_if_mdf_since);
        }
        else if( h1.cmpeqi("Set-Cookie") )
        {
            _set_cookie = h;
        }
        else {
            e = http.on_extra_header( h1, h );
            if(e) break;
        }
    }

    return e;
}

COID_NAMESPACE_END


#endif //__COID_COMM_HTTPSTREAM2__HEADER_FILE__
