
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
 * Brano Kemen
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

#ifndef __COID_COMM_MATHF__HEADER_FILE__
#define __COID_COMM_MATHF__HEADER_FILE__

#include <cmath>

constexpr double MC_E = 2.71828182845904523536;   // e
constexpr double MC_LOG2E = 1.44269504088896340736;   // log2(e)
constexpr double MC_LOG10E = 0.434294481903251827651;  // log10(e)
constexpr double MC_LN2 = 0.693147180559945309417;  // ln(2)
constexpr double MC_LN10 = 2.30258509299404568402;   // ln(10)
constexpr double MC_PI = 3.14159265358979323846;   // pi
constexpr double MC_PI_2 = 1.57079632679489661923;   // pi/2
constexpr double MC_PI_4 = 0.785398163397448309616;  // pi/4
constexpr double MC_1_PI = 0.318309886183790671538;  // 1/pi
constexpr double MC_2_PI = 0.636619772367581343076;  // 2/pi
constexpr double MC_2_SQRTPI = 1.12837916709551257390;   // 2/sqrt(pi)
constexpr double MC_SQRT2 = 1.41421356237309504880;   // sqrt(2)
constexpr double MC_SQRT1_2 = 0.707106781186547524401;  // 1/sqrt(2)


#include "namespace.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
inline float exp2f( float xx )
{
    return expf( xx * (float)0.69314718055994530942 );
}

////////////////////////////////////////////////////////////////////////////////
inline double exp2( double xx )
{
    return exp( xx * 0.69314718055994530942 );
}

////////////////////////////////////////////////////////////////////////////////
///Calculate return value such that x == (retval) * 10^exp10, retval >= 1 and < 10
inline double frexp10(double x, int *exp10)
{
    double xabs = fabs(x);
    if(x == 0.0) {
        *exp10=0;
        return 0.0;
    }

    double mantissa = log10(xabs);
    *exp10 = (int)floor(mantissa);

    mantissa = pow(10.0, mantissa - double(*exp10));
    if(x<0)
        mantissa = -mantissa;

    return mantissa;
}

////////////////////////////////////////////////////////////////////////////////
inline float approx_fast_sqrt(float fx)
{
#if defined(SYSTYPE_32) && defined(SYSTYPE_MSVC) && !defined(__clang__)
      float fret;
      __asm {

          mov eax,    fx
          sub eax,    0x3F800000
          sar eax,    1
          add eax,    0x3F800000
          mov [fret], eax
      }
      return fret;
#else
	return ::sqrtf(fx);
#endif
}

////////////////////////////////////////////////////////////////////////////////
inline float approx_inv_sqrt (float x)
{
    float xhalf = 0.5f*x;
    int i = *(int*)&x;
    i = 0x5f3759df - (i >> 1); // This line hides a LOT of math!
    x = *(float*)&i;
    x = x*(1.5f - xhalf*x*x); // repeat this statement for a better approximation
    return x;
}

COID_NAMESPACE_END


#endif //__COID_COMM_MATHF__HEADER_FILE__
