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

#ifndef __COID_COMM_PACKSTREAM__HEADER_FILE__
#define __COID_COMM_PACKSTREAM__HEADER_FILE__

#include "../namespace.h"

#include "binstream.h"
#include "../dynarray.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///Generic pack/unpack wrapping binstream
class packstream : public binstream
{
public:
    virtual ~packstream() { }

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return 0;
    }

    virtual opcd write_raw( const void* p, uints& len ) = 0;
    virtual opcd read_raw( void* p, uints& len ) = 0;

    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS )
    {   return ersUNAVAILABLE; }

    virtual opcd bind( binstream& bin, int io=0 )
    {
        if( io<0 )
            _in = &bin;
        else if( io>0 )
            _out = &bin;
        else
            _in = _out = &bin;
        return 0;
    }

    virtual bool is_open() const        { return _in->is_open(); }
    virtual void flush()                = 0;
    virtual void acknowledge (bool eat=false) = 0;

    virtual opcd close( bool linger=false ) = 0;

    packstream() : _in(0),_out(0)       { }
    packstream( binstream* bin, binstream* bout )
    {
        _in = bin;
        _out = bout;
    }

    packstream( binstream& bin )
    {
        _in = _out = &bin;
    }

protected:
    binstream* _in;                         //< underlying input (source) binstream
    binstream* _out;                        //< underlying output (destination) binstream
};

COID_NAMESPACE_END

#endif //__COID_COMM_PACKSTREAM__HEADER_FILE__
