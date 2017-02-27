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

#ifndef __COID_COMM_FILESTREAMGZ__HEADER_FILE__
#define __COID_COMM_FILESTREAMGZ__HEADER_FILE__

#include "../namespace.h"

#include "binstream.h"

#define _LARGEFILE64_SOURCE
#include "zlib.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///
class filestreamgz : public binstream
{
public:
    virtual ~filestreamgz()
    {
        close();
    }

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return 0;
    }

    virtual opcd peek_read( uint timeout ) {
        if(timeout)  return ersINVALID_PARAMS;
        
        uints n=0;
        return read_raw( 0, n );
    }

    virtual opcd peek_write( uint timeout )     { return 0; }

    virtual bool is_open() const                { return _gz != 0; }
    virtual void flush()                        { }
    virtual void acknowledge (bool eat=false)   { }


    /* The mode parameter is as
        in fopen ("rb" or "wb") but can also include a compression level ("wb9") or
        a strategy: 'f' for filtered data as in "wb6f", 'h' for Huffman-only
        compression as in "wb1h", 'R' for run-length encoding as in "wb1R", or 'F'
        for fixed code compression as in "wb9F".  (See the description of
        deflateInit2 for more information about the strategy parameter.) Also "a"
        can be used instead of "w" to request that the gzip stream that will be
        written be appended to the file. */
    virtual opcd open( const zstring& name, const token& attr = "rb" ) override
    {
        zstring zattr = attr;

        _gz = gzopen(name.c_str(), zattr.c_str());
        if(!_gz)
            return ersFAILED;

        return 0;
    }

    virtual opcd close( bool linger=false )
    {
        if(_gz) {
            gzclose(_gz);
            _gz = 0;
        }

        return 0;
    }

    virtual opcd seek( int seektype, int64 pos )
    {
        return gzseek(_gz, (ints)pos, (seektype&fSEEK_CURRENT)!=0 ? SEEK_CUR : SEEK_SET) >= 0
            ? ersNOERR
            : ersFAILED;
    }


    filestreamgz()
    {
        _gz = 0;
    }

    ///
    virtual opcd write_raw( const void* p, uints& len )
    {
        RASSERT(len <= UINT_MAX);
        int sr = gzwrite(_gz, p, uint(len));
        if(sr<0)
            return ersFAILED;

        len -= sr;
        return 0;
    }

    ///
    virtual opcd read_raw( void* p, uints& len )
    {
        RASSERT(len <= UINT_MAX);
        int sr = gzread(_gz, p, uint(len));
        if(sr<0)
            return ersFAILED;

        len -= sr;
        return 0;
    }

    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS )
    {   return ersUNAVAILABLE; }

    virtual void reset_read()
    {
        seek(0, 0);
    }

    virtual void reset_write()
    {
        seek(0, 0);
    }

    bool set_pos( int64 pos ) {
        return setpos(pos);
    }

protected:

    bool setpos( int64 pos )
    {
        return -1 != gzseek64(_gz, pos, SEEK_SET );
    }

    int64 getpos() const
    {
        return gztell64(_gz);
    }

protected:

    gzFile _gz;
};

COID_NAMESPACE_END

#endif //__COID_COMM_filestreamgz__HEADER_FILE__
