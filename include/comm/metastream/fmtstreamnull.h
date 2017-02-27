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
 * Outerra.
 * Portions created by the Initial Developer are Copyright (C) 2013
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

#ifndef __COID_COMM_FMTSTREAMNULL__HEADER_FILE__
#define __COID_COMM_FMTSTREAMNULL__HEADER_FILE__

#include "fmtstream.h"



COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
class fmtstreamnull : public fmtstream
{
public:
    fmtstreamnull()
    {}
    
    ~fmtstreamnull()
    {}

    virtual token fmtstream_name() override { return "fmtstreamnull"; }
    virtual void fmtstream_file_name( const token& file_name ) override {}

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    opcd write_key( const token& key, int kmember ) override
    {
        return 0;
    }

    opcd read_key( charstr& key, int kmember, const token& expected_key ) override
    {
        return ersNO_MORE;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    opcd write( const void* p, type t ) override
    {
        return 0;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////
    opcd read( void* p, type t ) override
    {
        if( t.type == type::T_STRUCTEND )
            return 0;
        else if( t.type == type::T_STRUCTBGN )
            return 0;
        else if( t.type == type::T_SEPARATOR )
            return ersNO_MORE;

        DASSERT(0); //should not get here
        return ersIMPROPER_STATE;
    }


    virtual opcd write_array_separator( type t, uchar end ) override
    {
        return 0;
    }

    virtual opcd read_array_separator( type t ) override
    {
        DASSERT(0); //should not get here
        return ersIMPROPER_STATE;
    }

    virtual opcd write_array_content( binstream_container_base& c, uints* count, metastream* m ) override
    {
        *count = c.count();
        return 0;
    }

    virtual opcd read_array_content( binstream_container_base& c, uints n, uints* count, metastream* m ) override
    {
        DASSERT(0); //should not get here
        return ersIMPROPER_STATE;
    }

    virtual void acknowledge( bool eat = false ) override
    {
    }
};


COID_NAMESPACE_END


#endif  // ! __COID_COMM_FMTSTREAMNULL__HEADER_FILE__
