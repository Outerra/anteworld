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
 * Brano Kemen.
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

#ifndef __COID_COMM_HPTIMER__HEADER_FILE__
#define __COID_COMM_HPTIMER__HEADER_FILE__

#include "namespace.h"
#include "commtypes.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////

class nsec_timer
{
	static uint64 _freq;
    static double _freqd;
    uint64 _start;
    int64 _dtns;

public:

    nsec_timer();

    //@return elapsed time in seconds
	double time();

    //@return elapsed time in nanoseconds
	uint64 time_ns();
    
    //@return current time in nanoseconds
    static uint64 current_time_ns();

    ///Adjust returned time
    void adjust_time_ns( int64 dtns ) {
        _dtns += dtns;
    }

    ///Reset timer (sets current time as the base for elapsed time methods)
	void reset();
};



////////////////////////////////////////////////////////////////////////////////
#ifdef SYSTYPE_WIN
extern "C" {
    __declspec(dllimport) uint __stdcall timeBeginPeriod( uint );
    __declspec(dllimport) uint __stdcall timeEndPeriod( uint );
    __declspec(dllimport) ulong __stdcall timeGetTime();
}
#endif


class msec_timer
{
    uint64 _tstart;
    uint _period;

public:
    msec_timer()            { _tstart = 0; _period = 1000; }
    ~msec_timer()           {}

    void set_period_usec( uint usec )   { _period = usec; }
    uint get_period_usec() const        { return _period; }

#ifdef SYSTYPE_WIN
    static void init()      { timeBeginPeriod(1); }
    static void term()      { timeEndPeriod(1); }
#else
    static void init()      {}
    static void term()      {}
#endif

    void reset();
    void set( uint msec );

    uint time() const;
    uint time_usec() const;
	uint time_reset();

    uint ticks() const;
    int usec_to_tick( uint k ) const;

    /// return time in miliseconds
	static uint get_time();
};

////////////////////////////////////////////////////////////////////////////////

COID_NAMESPACE_END



#define COID_TIME_POINT(name) uint64 ___timer_point_##name = coid::nsec_timer::current_time_ns()

#define COID_TIME_SINCE(name) \
    float(double(coid::nsec_timer::current_time_ns() - ___timer_point_##name) * 1e-9)
#define COID_TIME_BETWEEN(name1, name2) \
    float(fabs(double(int64(___timer_point_##name1 - ___timer_point_##name2)) * 1e-9))


#endif //#ifndef __COID_COMM_HPTIMER__HEADER_FILE__
