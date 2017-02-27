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
 * Robert Strycek
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


#include "retcodes.h"
#include "str.h"


COID_NAMESPACE_BEGIN

///
struct errorTABLE
{
    const opcd::errcode* code;
	short parent;
	const char* desc;
};

////////////////////////////////////////////////////////////////////////////////
/// position in this table must be equal to error number (see #defines in retcodes.h)
/// if parent is not set, its value is unique and less than zero (so it's easy to find out if parents are the same)
static errorTABLE opcdTABLE[opcdERROR_COUNT] =
{
	{(MKERR1("\0", "\0"))._ptr, 0, "OK"},	/// leave first entry empty

    {(ersUNKNOWN)._ptr, -1, "unknown error"},
    {(ersFRAMEWORK_ERROR)._ptr, -2, "framework error"}, /// 1
    {(ersSPECIFIC)._ptr, -3, "object specific"},        /// 2
    {(ersABORT)._ptr, -4, "aborted"},
    {(ersALREADY_DELETED)._ptr, -5, "object already deleted"},
    {(ersALREADY_EXISTS)._ptr, -6, "object already exists, cannot be created"},
    {(ersBUSY)._ptr, -7, "host busy"},
    {(ersDENIED)._ptr, -8, "request denied, insufficient access rights"},
    {(ersEXCEPTION)._ptr, -9, "internal exception"},
    {(ersEXIT)._ptr, -10, "exit"},
    {(ersFAILED)._ptr, -11, "failure (general)"},
    {(ersFAILED_ASSERTION)._ptr, -12, "assertion failed"},
    {(ersIGNORE)._ptr, -13, "ignore"},
    {(ersIMPROPER_STATE)._ptr, -14, "object is in improper state for the operation"},
    {(ersINTERNAL_ERROR)._ptr, -15, "internal error"},
    {(ersINVALID_CREDENTIALS)._ptr, -16, "invalid credentials"},
    {(ersINVALID_NAME)._ptr, -17, "invalid name or string"},
    {(ersINVALID_PARAMS)._ptr, -18, "invalid parameters"},
    {(ersINVALID_TYPE)._ptr, -19, "invalid type specified"},
    {(ersINVALID_VERSION)._ptr, -20, "invalid version"},
    {(ersINVALID_LOGIN)._ptr, -21, "invalid login"},       /// 20
    {(ersINVALID_LOGIN_NAME)._ptr, 21, "invalid login name"},
    {(ersINVALID_LOGIN_PASSWORD)._ptr, 21, "invalid password"},
    {(ersIO_ERROR)._ptr, -24, "io error during processing"},
    {(ersMISMATCHED)._ptr, -25, "mismatched (format of something)"},
    {(ersNO_CHANGE)._ptr, -26, "no change from last call (cache)"},
    {(ersNO_MATCH)._ptr, -27, "no object matches the requisites"},
    {(ersNO_MORE)._ptr, -28, "no more (data)"},
    {(ersNOT_ENOUGH_MEM)._ptr, -29, "not enough allocatable memory"},
    {(ersNOT_ENOUGH_VIRTUAL_MEM)._ptr, -30, "out of address space"},
    {(ersNOT_FOUND)._ptr, -31, "requested object not found"},
    {(ersNOT_IMPLEMENTED)._ptr, -32, "method not implemented"},
    {(ersNOT_READY)._ptr, -33, "object not ready (asynchronous)"},
    {(ersOBSOLETE)._ptr, -34, "obsolete (version)"},
    {(ersOUT_OF_RANGE)._ptr, -35, "index or value out of range"},
    {(ersREJECTED)._ptr, -36, "request rejected"},
    {(ersRETRY)._ptr, -37, "retry"},
    {(ersSYNTAX_ERROR)._ptr, -38, "syntax error, format invalid"},
    {(ersTIMEOUT)._ptr, -39, "timeout"},
    {(ersUNAVAILABLE)._ptr, -40, "operation is unavailable at this time/state"},
    {(ersUNKNOWN_CMD)._ptr, -41, "unknown/unrecognized command"},

    {(ersFE_ALREADY_CONNECTED)._ptr, 2, "already connected"},
    {(ersFE_CHANNEL)._ptr, 2, "error on communication link"},
    {(ersFE_DENIED)._ptr, 2, "connection denied"},
    {(ersFE_DISCONNECTED)._ptr, 2, "disconnected from source"},
    {(ersFE_EXCEPTION)._ptr, 2, "client side exception"},
    {(ersFE_INVALID_SERVER_VER)._ptr, 2, "invalid coid server version"},
    {(ersFE_INVALID_SVC_VER)._ptr, 2, "invalid service version"},
    {(ersFE_NO_ACCESS_MODE_FITS)._ptr, 2, "no access mode fits"},
    {(ersFE_NO_SUCH_SERVICE)._ptr, 2, "no such service"},
    {(ersFE_NONLOCAL_AUTO)._ptr, 2, "nonlocal autoclient"},
    {(ersFE_NOT_ATTACHED_SVC)._ptr, 2, "no such service attached (obj.id out of bounds)"},
    {(ersFE_NOT_AUTHENTIFIED)._ptr, 2, "not authentified"},
    {(ersFE_NOT_PRIMARY_SVC)._ptr, 2, "not a primary service"},
    {(ersFE_NOT_YET_READY)._ptr, 2, "not yet ready"},
    {(ersFE_SVC_ALREADY_DELETED)._ptr, 2, "service was already deleted"},
    {(ersFE_UNKNOWN_METHOD)._ptr, 2, "unknown method"},
    {(ersFE_UNREACHABLE)._ptr, 2, "host unreachable"},
    {(ersFE_UNRECG_REQUEST)._ptr, 2, "unrecognized request"},
    {(ersFE_UNKNOWN_ERROR)._ptr, 2, "unknown error"},

    {(ersINTEGER_OVERFLOW)._ptr, -42, "integer overflow during conversion"},
    {(ersNOT_KNOWN)._ptr, -43, "not known"},
    {(ersDISCONNECTED)._ptr, -44, "disconnected"},
    {(ersINVALID_SESSION)._ptr, -45, "invalid session identifer"},
};


////////////////////////////////////////////////////////////////////////////////
uints opcd::find_code( const char* c, uints len )
{
    if( 0 == strncmp(c,"OK",len) )  return 0;

    if( len>6 )  len=6;
    for( uints i=0; i<opcdERROR_COUNT; ++i )
        if( 0 == strncmp(c,(const char*)opcdTABLE[i].code->_desc,len) )  return i;

    return 1;   //ersUNKNOWN
}

////////////////////////////////////////////////////////////////////////////////
const char* opcd::error_desc() const
{
    return opcdTABLE[code()].desc;
}

////////////////////////////////////////////////////////////////////////////////
bool opcd::parent1_equal( uint c1, uint c2 )
{
    return uint(opcdTABLE[c1].parent) == c2;
}

////////////////////////////////////////////////////////////////////////////////
void opcd::set( uint e )
{
	if(!e)  _ptr = 0;
	else if( e < opcdERROR_COUNT ) _ptr = opcdTABLE[e].code;
	else _ptr = (ersUNKNOWN)._ptr;
}



COID_NAMESPACE_END
