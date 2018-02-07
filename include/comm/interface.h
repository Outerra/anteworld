
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

#ifndef __COID_COMM_INTERFACE__HEADER_FILE__
#define __COID_COMM_INTERFACE__HEADER_FILE__

#include "namespace.h"
#include "str.h"
#include "dynarray.h"
#include "regex.h"
#include "log/logger.h"

class intergen_interface;

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///A global register for interfaces, used by intergen
class interface_register
{
public:

    static bool register_interface_creator( const token& ifcname, void* creator );

    static void* get_interface_creator( const token& ifcname );

    //@param curpath current path
    //@param incpath absolute path or path relative to curpath
    //@param dst [out] result path
    //@param relpath [out] gets relative path from root. If null, relative incpath can only refer to a sub-path below curpath
    static bool include_path( const token& curpath, const token& incpath, charstr& dst, token* relpath );

    static const charstr& root_path();


    typedef ref<logmsg> (*fn_log_t)(log::type, const tokenhash&, const void*);
    typedef bool (*fn_acc_t)(const token&);

    static void setup( const token& path, fn_log_t log, fn_acc_t access );

    typedef iref<intergen_interface> (*wrapper_fn)(void*, intergen_interface*);

    ///Get interface wrapper creator matching the given name
    //@param name interface creator name in the format [ns1::[ns2:: ...]]::class
    static wrapper_fn get_interface_wrapper( const token& name );

    ///Get interface maker creator matching the given name
    //@param name interface creator name in the format [ns1::[ns2:: ...]]::class
    //@param script script type
    static void* get_interface_maker( const token& name, const token& script );

    ///Get client interface creator matching the given name
    //@param client client name
    //@param iface interface creator name in the format [ns1::[ns2:: ...]]::class
    //@param module required module to match
    static wrapper_fn get_interface_client( const token& client, const token& iface, const token& module );

    struct creator {
        token name;
        void* creator_ptr;
    };

    ///Get interface creators matching the given name
    //@param name interface creator name in the format [ns1::[ns2:: ...]]::class[.creator]
    //@param script script type (js, lua ...), if empty anything matches
    //@return array of interface creators for given script type (with script_handle argument)
    static dynarray<creator>& get_interface_creators( const token& name, const token& script, dynarray<creator>& dst );

    ///Get script interface creators matching the given name
    //@param name interface creator name in the format [ns1::[ns2:: ...]]::class[.creator]
    //@param script script type (js, lua ...), if empty anything matches
    //@return array of interface creators for given script type (with native script lib argument)
    static dynarray<creator>& get_script_interface_creators( const token& name, const token& script, dynarray<creator>& dst );

    ///Find interfaces containing given string
    static dynarray<creator>& find_interface_creators( const regex& str, dynarray<creator>& dst );

    static ref<logmsg> canlog( log::type type, const tokenhash& hash, const void* inst = 0 );

#ifdef COID_VARIADIC_TEMPLATES

    ///Formatted log message
    //@param type log level
    //@param hash source identifier (used for filtering)
    //@param fmt @see charstr.print
    template<class ...Vs>
    static void print( log::type type, const tokenhash& hash, const token& fmt, Vs&&... vs )
    {
        ref<logmsg> msgr = canlog(type, hash);
        if(!msgr)
            return;

        charstr& str = msgr->str();
        str.print(fmt, std::forward<Vs>(vs)...);
    }

#endif //COID_VARIADIC_TEMPLATES
};


COID_NAMESPACE_END

#endif //__COID_COMM_INTERFACE__HEADER_FILE__
