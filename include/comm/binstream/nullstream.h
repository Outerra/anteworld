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

#ifndef __COMM_NULLSTREAM__HEADER_FILE__
#define __COMM_NULLSTREAM__HEADER_FILE__

#include "../namespace.h"

#include "binstream.h"

COID_NAMESPACE_BEGIN

class nullstream_t : public binstream
{
public:

    virtual uint binstream_attributes( bool in0out1 ) const
    {
        return fATTR_NO_INPUT_FUNCTION;
    }

    virtual opcd write_raw( const void*, uints& len )
    {
        len = 0;
        return 0;
    }

    virtual opcd read_raw( void*, uints& len )
    {   return ersNO_MORE; }

    virtual opcd read_until( const substring& ss, binstream* bout, uints max_size=UMAXS )
    {   return ersUNAVAILABLE; }

    virtual opcd peek_read( uint timeout )          { return ersNO_MORE; }
    virtual opcd peek_write( uint timeout )         { return 0; }

    virtual opcd bind( binstream& bin, int io=0 )   { return 0; }

    virtual bool is_open() const                    { return true; }
    virtual void flush()                            { }
    virtual void acknowledge( bool eat=false )      { }

    virtual void reset_read()                       { }
    virtual void reset_write()                      { }
};

static class nullstream_t     nullstream;

COID_NAMESPACE_END

#endif //__COMM_NULLSTREAM__HEADER_FILE__

