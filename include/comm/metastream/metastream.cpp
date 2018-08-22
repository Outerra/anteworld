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
 * Portions created by the Initial Developer are Copyright (C) 2006
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

#include "metastream.h"
#include "../local.h"
#include "../hash/hashkeyset.h"
#include "../log/logger.h"

COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
void metastream::warn_obsolete(const token& name)
{
    (_err = "warning: obsolete variable '") << name << '\'';
    fmt_error(false);
    coidlog_warning("metastream", _err);
}

////////////////////////////////////////////////////////////////////////////////

struct SMReg {
    typedef hash_keyset<MetaDesc,_Select_Copy<MetaDesc,token>> map_t;
    map_t _map;

    dynarray<local<MetaDesc>> _anon;
};

////////////////////////////////////////////////////////////////////////////////
void metastream::structure_map::get_all_types( dynarray<const MetaDesc*>& dst ) const
{
    SMReg& smr = *(SMReg*)pimpl;

    SMReg::map_t::const_iterator b,e;
    b = smr._map.begin();
    e = smr._map.end();

    dst.reset();
    dst.reserve( smr._map.size(), false );
    for(; b!=e; ++b )
        *dst.add() = &(*b);
}

////////////////////////////////////////////////////////////////////////////////
metastream::structure_map::structure_map()
{
    pimpl = new SMReg;
}

////////////////////////////////////////////////////////////////////////////////
metastream::structure_map::~structure_map()
{
    delete (SMReg*)pimpl;
    pimpl = 0;
}

////////////////////////////////////////////////////////////////////////////////
MetaDesc* metastream::structure_map::insert( MetaDesc&& v )
{
    SMReg& smr = *(SMReg*)pimpl;

    return (MetaDesc*) smr._map.insert_value(std::forward<MetaDesc>(v));
}

////////////////////////////////////////////////////////////////////////////////
MetaDesc* metastream::structure_map::insert_anon()
{
    SMReg& smr = *(SMReg*)pimpl;

    return (*smr._anon.add() = new MetaDesc()).ptr();
}

////////////////////////////////////////////////////////////////////////////////
MetaDesc* metastream::structure_map::find( const token& k ) const
{
    const SMReg& smr = *(const SMReg*)pimpl;

    const MetaDesc* md = smr._map.find_value(k);
    return (MetaDesc*)md;
}

COID_NAMESPACE_END
