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
 * Portions created by the Initial Developer are Copyright (C) 2005
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





//NOT IMPLEMENTED YET!





#ifndef __COID_COMM_PACKSTREAMLZO__HEADER_FILE__
#define __COID_COMM_PACKSTREAMLZO__HEADER_FILE__

#include "../namespace.h"

#include "packstream.h"
#include "cachestream.h"

#include "minilzo.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///
class packstreamlzo : public packstream
{
    enum {
        BLOCK_SIZE              = 1024,
    };

    ////////////////////////////////////////////////////////////////////////////////
    class lzocachestream : public cachestream
    {
        packstreamlzo* _parent;

        lzocachestream( packstreamlzo* par ) : _parent(par)
        {
            reserve_buffer_size(BLOCK_SIZE);
        }

        virtual opcd on_cache_flush( void* p, uints size, bool final )
        {
            return _parent->cache_flush( p, size, final );
        }

        virtual uints on_cache_fill( void* p, uints size )
        {
            return _parent->cache_fill( p, size );
        }
    };

public:
    virtual ~packstreamlzo()
    {
        packed_flush();
        inflateEnd( &_strin );
        deflateEnd( &_strout );
    }

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return 0;
    }
/*
    virtual opcd peek_read( uint timeout ) {
        return _cache.peek_read(timeout);
    }

    virtual opcd peek_write( uint timeout ) {
        return _cache.peek_write(timeout);
    }
*/
    virtual bool is_open() const        { return _in->is_open(); }

    virtual void flush()
    {
    }
    virtual void acknowledge (bool eat=false)
    {
    }

    packstreamlzo()
    {
    }
    packstreamlzo( binstream* bin, binstream* bout ) : packstream(bin,bout)
    {
    }

protected:

    void init_streams()
    {
        _wrkbuf.need_new( LZO1X_MEM_COMPRESS );
    }

    ///
    virtual opcd write_raw( const void* p, uints& len )
    {
        return _cache->write_raw(p,len);
    }

    ///
    virtual opcd read_raw( void* p, uints& len )
    {
        return _cache->read_raw(p,len);
    }

    opcd cache_flush( void* p, uints size, bool final )
    {
        //lzo1x_1_compress( p, size, )
    }

    uints cache_fill( void* p, uints size )
    {
    }
    
    dynarray<uchar> _lzowrkbuf;
    cachestream _cache;
};

COID_NAMESPACE_END

#endif //__COID_COMM_PACKSTREAMLZO__HEADER_FILE__
