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

#ifndef __COMM_STDSTREAM_HEADER_FILE__
#define __COMM_STDSTREAM_HEADER_FILE__

#include "../namespace.h"

#include "stlstream.h"
#include "txtstream.h"

COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
class stdoutstream : public txtstream
{
public:
    stdoutstream()
    {
        create_internal_buffer();
        flush_on_character('\n');
    }

    virtual ~stdoutstream()
    {
        flush();
    }

    virtual void flush()
    {
        txtstream::flush();
        binstreambuf& buf = *get_read_buffer();

#if defined(SYSTYPE_WIN) && (!defined(_CONSOLE) || defined(STDSTREAM_DEBUG_OUT))
        dynarray<char>& dyn = buf.get_buf();
        *dyn.add() = 0;
        debug_out(dyn.ptr());
        dyn.resize(-1);
#endif
        token t = buf;

        fwrite(t.ptr(), 1, t.len(), stdout);
        reset_write();
    }

    static void debug_out( const char* );
};

COID_NAMESPACE_END


#endif //__COMM_STDSTREAM_HEADER_FILE__
