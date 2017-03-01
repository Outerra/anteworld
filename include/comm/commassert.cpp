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

#include "commassert.h"
#include "sync/mutex.h"
#include "binstream/filestream.h"
#include "binstream/txtstream.h"
#include "binstream/nullstream.h"

COID_NAMESPACE_BEGIN


struct coid_assert_log
{
    COIDNEWDELETE_NOTRACK

    bofstream _file;
    txtstream _text;
    comm_mutex _mutex;


    binstream& get_file()
    {
        if(!_file.is_open())
        {
            _file.filestream::open("assert.log","wct");
            _text.bind(_file);
        }

        if( _file.is_open() )
            return _text;
        return nullstream;
    }

    bool is_open() const {
        return _file.is_open();
    }

    coid_assert_log() : _mutex(10, false)
    {}
};

static binstream& bin = SINGLETON(coid_assert_log)._text;

static int __assert_throws = 1;

////////////////////////////////////////////////////////////////////////////////
opcd __rassert( const char* txt, opcd exc, const char* file, int line, const char* expr )
{
    coid_assert_log& asl = SINGLETON(coid_assert_log);
    {
        comm_mutex_guard<comm_mutex> _guard( asl._mutex );
        asl.get_file();

        if(&bin)
            bin << "Assertion failed in " << file << ":" << line << " expression:\n    "
		    << expr << "\n    " << (txt ? txt : "") << "\n\n"
            << BINSTREAM_FLUSH;
    }

    opcd e = __assert_throws ? exc : opcd(0);
    return e;
}


COID_NAMESPACE_END
