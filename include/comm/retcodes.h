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
 * Robert Strycek
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

#ifndef __COID_COMM_RETCODES__HEADER_FILE__
#define __COID_COMM_RETCODES__HEADER_FILE__

#include "namespace.h"
#include "comm.h"

COID_NAMESPACE_BEGIN


///Handle exceptions in expression, convert to return the opcd code. ThreadException rethrown.
#define OPCD_CATCH_RETURN(exp) \
    try { exp; } \
    catch( opcd __e )  {  return __e;  } \
    catch( thread::Exception& )  {  throw;  } \
    catch( ... )  {  return ersEXCEPTION "non-opcd type thrown";  }


#define OPCD_CATCH_ERR(exp,e) \
    e = 0; \
    try { exp; } \
    catch( opcd __e )  {  e = __e;  } \
    catch( thread::Exception& )  {  throw;  } \
    catch( ... )  {  e = ersEXCEPTION "non-opcd type thrown";  } \

#define OPCD_CATCH(exp) \
    try { exp; } \
    catch( opcd )  { } \
    catch( thread::Exception& )  {  throw;  } \
    catch( ... )  { }

#define ERETURN(exp)    { coid::opcd e = exp;  if(e)  return e; }



////////////////////////////////////////////////////////////////////////////////
#define MKERR1(id, code)                   (coid::opcd)(const coid::opcd::errcode*) id "\x000" code
#define MKERR2(id1, id2, code)             (coid::opcd)(const coid::opcd::errcode*) id1 id2 code


#define ersxFRAMEWORK_ERROR(id, code)       MKERR1(id, "FE" code)
#define ersxSPECIFIC(id, code)              MKERR1(id, "SP" code)



#define NOERR                               0
#define ersNOERR                            coid::opcd(0)


#define ersUNKNOWN                          MKERR1("\x001", "?????")
#define ersFRAMEWORK_ERROR                  MKERR1("\x002", "FrErr")	//2
#define ersSPECIFIC                         MKERR1("\x003", "Spcfc")
#define ersABORT                            MKERR1("\x004", "Abort")
#define ersALREADY_DELETED                  MKERR1("\x005", "ADel ")
#define ersALREADY_EXISTS                   MKERR1("\x006", "Exist")
#define ersBUSY                             MKERR1("\x007", "Busy ")
#define ersDENIED                           MKERR1("\x008", "Deny ")
#define ersEXCEPTION                        MKERR1("\x009", "Exc  ")
#define ersEXIT                             MKERR1("\x00A", "Exit ")
#define ersFAILED                           MKERR1("\x00B", "Fail ")
#define ersFAILED_ASSERTION                 MKERR1("\x00C", "Assrt")
#define ersIGNORE                           MKERR1("\x00D", "Ignor")
#define ersIMPROPER_STATE                   MKERR1("\x00E", "State")
#define ersINTERNAL_ERROR                   MKERR1("\x00F", "IntEr")
#define ersINVALID_CREDENTIALS              MKERR1("\x010", "ICred")
#define ersINVALID_NAME                     MKERR1("\x011", "IName")
#define ersINVALID_PARAMS                   MKERR1("\x012", "IParm")
#define ersINVALID_TYPE                     MKERR1("\x013", "IType")
#define ersINVALID_VERSION                  MKERR1("\x014", "IVer ")
#define ersINVALID_LOGIN                    MKERR1("\x015", "ILgn ")	//21
#define ersINVALID_LOGIN_NAME               MKERR1("\x016", "LGnam")
#define ersINVALID_LOGIN_PASSWORD           MKERR1("\x017", "LGpwd")
#define ersIO_ERROR                         MKERR1("\x018", "IOerr")
#define ersMISMATCHED                       MKERR1("\x019", "Misma")
#define ersNO_CHANGE                        MKERR1("\x01A", "NChng")
#define ersNO_MATCH                         MKERR1("\x01B", "NMtch")
#define ersNO_MORE                          MKERR1("\x01C", "NMore")
#define ersNOT_ENOUGH_MEM                   MKERR1("\x01D", "!Mem ")
#define ersNOT_ENOUGH_VIRTUAL_MEM           MKERR1("\x01E", "!VMem")
#define ersNOT_FOUND                        MKERR1("\x01F", "!Fnd ")
#define ersNOT_IMPLEMENTED                  MKERR1("\x020", "!Impl")
#define ersNOT_READY                        MKERR1("\x021", "!Rdy ")
#define ersOBSOLETE                         MKERR1("\x022", "Obslt")
#define ersOUT_OF_RANGE                     MKERR1("\x023", "OoRng")
#define ersREJECTED                         MKERR1("\x024", "Rjctd")
#define ersRETRY                            MKERR1("\x025", "Retry")
#define ersSYNTAX_ERROR                     MKERR1("\x026", "SynEr")
#define ersTIMEOUT                          MKERR1("\x027", "TimeO")
#define ersUNAVAILABLE                      MKERR1("\x028", "!Avlb")
#define ersUNKNOWN_CMD                      MKERR1("\x029", "?Cmd ")	//41

/// FRAMEWORK_ERROR subcodes:
#define ersFE_ALREADY_CONNECTED             ersxFRAMEWORK_ERROR("\x02A", "con")
#define ersFE_CHANNEL                       ersxFRAMEWORK_ERROR("\x02B", "chn")
#define ersFE_DENIED                        ersxFRAMEWORK_ERROR("\x02C", "den")
#define ersFE_DISCONNECTED                  ersxFRAMEWORK_ERROR("\x02D", "dis")
#define ersFE_EXCEPTION                     ersxFRAMEWORK_ERROR("\x02E", "exc")
#define ersFE_INVALID_SERVER_VER            ersxFRAMEWORK_ERROR("\x02F", "vSr")
#define ersFE_INVALID_SVC_VER               ersxFRAMEWORK_ERROR("\x030", "vSc")
#define ersFE_NO_ACCESS_MODE_FITS           ersxFRAMEWORK_ERROR("\x031", "acc")
#define ersFE_NO_SUCH_SERVICE               ersxFRAMEWORK_ERROR("\x032", "nss")
#define ersFE_NONLOCAL_AUTO                 ersxFRAMEWORK_ERROR("\x033", "nla")
#define ersFE_NOT_ATTACHED_SVC              ersxFRAMEWORK_ERROR("\x034", "!at")
#define ersFE_NOT_AUTHENTIFIED              ersxFRAMEWORK_ERROR("\x035", "!au")
#define ersFE_NOT_PRIMARY_SVC               ersxFRAMEWORK_ERROR("\x036", "!pr")
#define ersFE_NOT_YET_READY                 ersxFRAMEWORK_ERROR("\x037", "!rd")
#define ersFE_SVC_ALREADY_DELETED           ersxFRAMEWORK_ERROR("\x038", "dlt")
#define ersFE_UNKNOWN_METHOD                ersxFRAMEWORK_ERROR("\x039", "?m ")
#define ersFE_UNREACHABLE                   ersxFRAMEWORK_ERROR("\x03A", "rch")
#define ersFE_UNRECG_REQUEST                ersxFRAMEWORK_ERROR("\x03B", "rcg")
#define ersFE_UNKNOWN_ERROR                 ersxFRAMEWORK_ERROR("\x03C", "???")   //60

#define ersINTEGER_OVERFLOW                 MKERR1("\x03D", "intOv")
#define ersNOT_KNOWN                        MKERR1("\x03E", "!know")
#define ersDISCONNECTED                     MKERR1("\x03f", "discn")
#define ersINVALID_SESSION                  MKERR1("\x040", "Isess")

//ADJUST THIS WHEN ADDING ERROR CODES
#define opcdERROR_COUNT                     0x41



////////////////////////////////////////////////////////////////////////////////
///Error code structure
struct opcd
{
    ///Internal error code struct
    struct errcode
    {
        short _code;
        unsigned char _desc[6];

		int code() const                { return _code; }
    };

    typedef const errcode* perrcode_t;

    const errcode* _ptr;



    ////////////////////////////////////////////////////////////////////////////////
    opcd() : _ptr(0) {}

    opcd( const opcd& e ) : _ptr(e._ptr) {}
    opcd( const errcode* pe ) : _ptr(pe) {}

    opcd& operator = (const opcd e)
    {
        _ptr = e._ptr;
        return *this;
    }

    opcd& operator = (const errcode* pe)
    {
        _ptr = pe;
        return *this;
    }


    ///Set error by number
    void set( uint e );


    typedef perrcode_t opcd::* unspecified_bool_type;

    ///Automatic cast to bool for checking nerror
    operator unspecified_bool_type () const             { return _ptr ? &opcd::_ptr : 0; }
    bool operator ! () const                            { return _ptr == NULL; }

    bool operator == ( opcd c ) const
	{
		return code() == c.code();
        //if( _ptr == c._ptr ) return true;
        //if( _ptr == NULL || c._ptr == NULL ) return false;
		//return _ptr->code() == c._ptr->code();
	}

    bool operator != ( opcd c ) const                   { return ! (*this == c); }

	// returns true if right side is either parent or if both are equal
    bool operator >= ( opcd c ) const
	{
		uint c1 = code();
		uint c2 = c.code();
		if( c1 == c2 ) return true;
		return parent1_equal(c1,c2);
	}


    friend inline bool operator == (opcd c, int err)    { return (int) c.code() == err; }
    friend inline bool operator == (int err, opcd c)    { return (int) c.code() == err; }
    friend inline bool operator != (opcd c, int err)    { return (int) c.code() != err; }
    friend inline bool operator != (int err, opcd c)    { return (int) c.code() != err; }
//    friend inline bool operator >= (opcd c, int err)    { return c.actual() >= errcode(err); }
//    friend inline bool operator >= (int err, opcd c)    { return errcode(err) >= c.actual(); }


    ///Get specific error text
    const char* text() const
	{
        if( !_ptr || !_ptr->_code ) return "";
		return _ptr->_desc[5] ? (const char*)_ptr+7 : "";
	}

    ///Get common error text
    const char* error_desc() const;


    ///Return error code, max 5 characters valid or until 0
    const char* error_code() const
    {
        if( !_ptr || !_ptr->_code )  return "OK";
        return (const char*)_ptr->_desc;
    }


    static uints find_code( const char* c, uints len );


    uint code() const
    {
        if(!_ptr) return 0;
        if( _ptr->code() >= opcdERROR_COUNT ) return (ersUNKNOWN)._ptr->code();
		///TODO: remove this check later:
		//assert( _ptr->code() == opcd::opcdTABLE[_ptr->code()].code->code() );
        return _ptr->code();
    }

private:

    static bool parent1_equal( uint c1, uint c2 );
};


COID_NAMESPACE_END

#endif  /// ! __COID_COMM_RETCODES__HEADER_FILE__
