 #pragma once
#ifndef __OUTERRA_ENG_GLM_EXT_H__
#define __OUTERRA_ENG_GLM_EXT_H__

#define GLM_FORCE_INLINE
#define _USE_MATH_DEFINES

#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/swizzle.hpp>
#include <glm/gtx/verbose_operator.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/simd_vec4.hpp>

#include "glm_types.h"

#define swizzle_xyz() swizzle(glm::X, glm::Y, glm::Z)
#define s_xyz() swizzle(glm::X, glm::Y, glm::Z)

#define simd_align __declspec(align(16))

namespace glm {

/// clamped sqrt returning 0 on negative values
template<class T>
inline T sqrtc( T v ) { return v<0 ? T(0.0) : sqrt(v); }

/// clamped acos
template<class T>
inline T acosc( T v ) {
    return acos( v<-1 ? -1 : (v>1 ? 1 : v) );
}

///IEEE float representation
union ieee_float
{
    struct {
        uint base : 23;
        uint exponent : 8;
        uint sign : 1;
    };
    float f;
};

///IEEE double representation
union ieee_double
{
    struct {
        uint64 base : 52;
        uint64 exponent : 11;
        uint64 sign : 1;
    };
    double f;
};

///Faster computation of ldexp
inline float ldexp_fast( float v, int exp )
{
    ieee_float u;
    u.f = 2.0f;
    u.exponent += exp - 1;
    return v * u.f;
}

///Faster computation of ldexp
inline double ldexp_fast( double v, int exp )
{
    ieee_double u;
    u.f = 2.0;
    u.exponent += exp - 1;
    return v * u.f;
}

inline float ldexp_fast2f( int exp )
{
    ieee_float u;
    u.f = 2.0f;
    u.exponent += exp - 1;
    return u.f;
}

inline double ldexp_fast2d( int exp )
{
    ieee_double u;
    u.f = 2.0;
    u.exponent += exp - 1;
    return u.f;
}


inline float atan2_fast( float y, float x )
{
    const float pi14 = float(M_PI / 4.0);
    const float pi34 = 3.0f * pi14;
    float ay = fabs(y);
    float r, angle;
    if(x >= 0) {
        r = (x - ay) / (x + ay);
        angle = pi14;
    } else {
        r = (x + ay) / (ay - x);
        angle = pi34;
    }
    //angle -= pi14 * r;
    angle += (0.1963f * r * r - 0.9817f) * r;

    return y < 0 ? -angle : angle;
}

/*
typedef lowp_ivec4 short4;
typedef lowp_ivec3 short3;
typedef lowp_ivec2 short2;

typedef lowp_uvec4 ushort4;
typedef lowp_uvec3 ushort3;
typedef lowp_uvec2 ushort2;

typedef detail::tvec4<char> char4;
typedef detail::tvec3<char> char3;
typedef detail::tvec2<char> char2;

typedef detail::tvec4<uchar> uchar4;
typedef detail::tvec3<uchar> uchar3;
typedef detail::tvec2<uchar> uchar2;
*/

const float TWO_PI=6.28318530717958647692f;
const float PI=3.14159265358979323846f; //3.14159265358979323846264338327950288419716939937510;
const float PI_OVER_2=1.57079632679489661923f;
const float PI_OVER_4=0.78539816339744830962f;


template<typename T>
inline T length_squared( const detail::tvec2<T> &v ) { return glm::dot(v, v); }

template<typename T>
inline T length_squared( const detail::tvec3<T> &v ) { return glm::dot(v, v); }

template<typename T>
inline T length_squared( const detail::tvec4<T> &v ) { return glm::dot(v, v); }

///
inline float distance_to_segment(const glm::vec3 &p,const glm::vec3 &p0,const glm::vec3 &p1)
{
    vec3 v=p1-p0;
    vec3 w=p-p0;

    float c1=dot(w,v);
    if(c1<=0.0f)
		return distance(p,p0);

    float c2=dot(v,v);
    if(c2<=c1)
		return distance(p,p1);

    float b=c1/c2;
	vec3 Pb=p0+b*v;

	return distance(p,Pb);
}

///
inline float distance_to_line(const vec3 &p,const glm::vec3 &p0,const glm::vec3 &p1)
{
    vec3 v=p1-p0;
    vec3 w=p-p0;

    float c1=dot(w,v);
    float c2=dot(v,v);
    float b=c1/c2;

    vec3 Pb=p0+b*v;
    return distance(p,Pb);
}

struct aabb
{
	vec3 _min;
	vec3 _max;

	void extend(const vec3 &point) {

	}
};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//@return signed square root of absolute value
template<class T>
inline T signsqrt(T v)
{
    T av = sqrt(fabs(v));
    return v < 0 ? -av : av;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

///
template<class T>
inline bool is_equal(const T& a, const T& b, const T& tolerance)
{
	return bool(abs(a - b) <= tolerance);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline detail::tvec2<T> normalize0(const detail::tvec2<T>& v)
{
    T len = length(v);
    return len > 0
        ? v / len
        : v;
}

template<typename T>
inline detail::tvec3<T> normalize0(const detail::tvec3<T>& v)
{
    T len = length(v);
    return len > 0
        ? v / len
        : v;
}

template<typename T>
inline detail::tvec4<T> normalize0(const detail::tvec4<T>& v)
{
    T len = length(v);
    return len > 0
        ? v / len
        : v;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// 1 - 2*qy2 - 2*qz2	2*qx*qy - 2*qz*qw	2*qx*qz + 2*qy*qw
// 2*qx*qy + 2*qz*qw	1 - 2*qx2 - 2*qz2	2*qy*qz - 2*qx*qw
// 2*qx*qz - 2*qy*qw	2*qy*qz + 2*qx*qw	1 - 2*qx2 - 2*qy2

//@return quaternion multiplied with x=1 vector
template<typename T>
inline detail::tvec3<T> mul_quat_x(const detail::tquat<T>& q)
{
    return detail::tvec3<T>(
        1 - 2*(q.y*q.y + q.z*q.z),
        2 * (q.x*q.y + q.w*q.z),
        2 * (q.x*q.z - q.w*q.y));
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//@return quaternion multiplied with y=1 vector
template<typename T>
inline detail::tvec3<T> mul_quat_y(const detail::tquat<T>& q)
{
    return detail::tvec3<T>(
        2 * (q.x*q.y - q.w*q.z),
        1 - 2*(q.x*q.x + q.z*q.z),
        2 * (q.y*q.z + q.w*q.x));
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//@return quaternion multiplied with z=1 vector
template<typename T>
inline detail::tvec3<T> mul_quat_z(const detail::tquat<T>& q)
{
    return detail::tvec3<T>(
        2 * (q.x*q.z + q.w*q.y),
        2 * (q.y*q.z - q.w*q.x),
        1 - 2*(q.x*q.x + q.y*q.y));
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/// angle in radians and vector has to be normalized
template<typename T>
inline detail::tquat<T> make_quat(T angle, const detail::tvec3<T> &v)
{
    angle *= T(0.5);

    typename detail::tquat<T>::value_type hsin = sin(angle);
	return detail::tquat<T>(cos(angle), v.x * hsin, v.y * hsin, v.z * hsin);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/// creates quaternion rotation around X axis
template<typename T>
inline detail::tquat<T> make_quat_x(const T angle)
{
	typename detail::tquat<T>::value_type hsin = sin(angle * T(0.5));
	return detail::tquat<T>(cos(angle * T(0.5)), hsin, 0, 0);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/// creates quaternion rotation around Y axis
template<typename T>
inline detail::tquat<T> make_quat_y(const T angle)
{
	typename detail::tquat<T>::value_type hsin = sin(angle * T(0.5));
	return detail::tquat<T>(cos(angle * T(0.5)), 0, hsin, 0);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/// creates quaternion rotation around Z axis
template<typename T>
inline detail::tquat<T> make_quat_z(const T angle)
{
	typename detail::tquat<T>::value_type hsin = sin(angle * T(0.5));
	return detail::tquat<T>(cos(angle * T(0.5)), 0, 0, hsin);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/// vector has to be normalized
template<typename T>
inline detail::tquat<T> make_quat(
    const T cos_angle,
    const T sin_angle,
    const detail::tvec3<T> &v)
{
    /*T hcos = (1.0f + cos_angle) * 0.5f;
    hcos = sign(hcos) * sqrt(abs(hcos));
    const T hsin = (sin_angle / hcos) * 0.5f;
	return detail::tquat<T>(hcos, v.x * hsin, v.y * hsin, v.z * hsin);*/
	const T hcos = sqrtc((T(1.0) + cos_angle) * T(0.5));
	const T hsin = (sin_angle>0 ? 1 : -1) * sqrtc((T(1) - cos_angle) * T(0.5));
	return detail::tquat<T>(hcos, v.x * hsin, v.y * hsin, v.z * hsin);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/// both vectors have to be normalized!
//@param plane optional rotation plane normal (if known) for the degenerate cases
template<typename T>
inline detail::tquat<T> make_quat(
    const detail::tvec3<T> &from,
    const detail::tvec3<T> &to,
    const detail::tvec3<T>* plane=0)
{
	T cangle = dot(from, to);
    detail::tvec3<T> h;

    if(is_equal(cangle, T(-1.0), T(0.00005))) {
        if(plane)
            h = normalize(cross(from, *plane) + cross(*plane, to));
        else
            h = normalize(cross(from, detail::tvec3<T>(to.x + T(0.3), to.y - T(0.15), to.z - T(0.15))));
    }
    else
        h = normalize(from+to);

    //optimized: http://physicsforgames.blogspot.sk/2010/03/quaternion-tricks.html
    return detail::tquat<T>(
        dot(from,h),
        from.y*h.z - from.z*h.y,
        from.z*h.x - from.x*h.z,
        from.x*h.y - from.y*h.x);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

///Make a quaternion that rotates z- to the fwd vector and y+ to the up vector.
///fwd vector is internally adjusted to be orthogonal with up
template<typename T>
inline detail::tquat<T> make_quat_zy_align_fwd( const detail::tvec3<T>& fwd, const detail::tvec3<T>& up )
{
    detail::tvec3<T> side = cross(up, fwd);
    detail::tvec3<T> dir = normalize(cross(side, up));

    detail::tquat<T> qup = make_quat(detail::tvec3<T>(0,1,0), up, &detail::tvec3<T>(0,0,-1));
    detail::tvec3<T> xfwd = -mul_quat_z(qup);

    T cangle = glm::dot(xfwd, dir);
    if(is_equal(cangle, T(-1.0), T(0.00005)))
        //rotate around the other axis to avoid uncertainty
        return make_quat(cangle, -dot(side,xfwd), up) * qup;
    else
        return make_quat(xfwd, dir) * qup;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

///Make a quaternion that rotates z- to the fwd vector and y+ to the up vector.
///up vector is internally adjusted to be orthogonal with fwd
template<typename T>
inline detail::tquat<T> make_quat_zy_align_up( const detail::tvec3<T>& fwd, const detail::tvec3<T>& up )
{
    detail::tvec3<T> side = cross(up, fwd);
    detail::tvec3<T> nup = normalize(cross(fwd, side));

    detail::tquat<T> qup = make_quat(detail::tvec3<T>(0,1,0), nup, &detail::tvec3<T>(0,0,-1));
    detail::tvec3<T> xfwd = -mul_quat_z(qup);

    T cangle = glm::dot(xfwd, fwd);
    if(is_equal(cangle, T(-1.0), T(0.00005)))
        //rotate around the other axis to avoid uncertainty
        return make_quat(cangle, -dot(side,xfwd), nup) * qup;
    else
        return make_quat(xfwd, fwd) * qup;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

///Make a quaternion that rotates y+ to the fwd vector and z+ to the up vector.
///fwd vector is internally adjusted to be orthogonal with up
template<typename T>
inline detail::tquat<T> make_quat_yz_align_fwd( const detail::tvec3<T>& fwd, const detail::tvec3<T>& up )
{
    detail::tvec3<T> side = cross(up, fwd);
    detail::tvec3<T> dir = normalize(cross(side, up));

    detail::tquat<T> qup = make_quat(detail::tvec3<T>(0,0,1), up, &detail::tvec3<T>(0,1,0));
    detail::tvec3<T> xfwd = mul_quat_y(qup);

    T cangle = glm::dot(xfwd, dir);
    if(is_equal(cangle, T(-1.0), T(0.00005)))
        //rotate around the other axis to avoid uncertainty
        return make_quat(cangle, -dot(side,xfwd), up) * qup;
    else
        return make_quat(xfwd, dir) * qup;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

///Make a quaternion that rotates y+ to the fwd vector and z+ to the up vector.
///up vector is internally adjusted to be orthogonal with fwd
template<typename T>
inline detail::tquat<T> make_quat_yz_align_up( const detail::tvec3<T>& fwd, const detail::tvec3<T>& up )
{
    detail::tvec3<T> side = cross(up, fwd);
    detail::tvec3<T> nup = normalize(cross(fwd, side));

    detail::tquat<T> qup = make_quat(detail::tvec3<T>(0,0,1), nup, &detail::tvec3<T>(0,1,0));
    detail::tvec3<T> xfwd = mul_quat_y(qup);

    T cangle = glm::dot(xfwd, fwd);
    if(is_equal(cangle, T(-1.0), T(0.00005)))
        //rotate around the other axis to avoid uncertainty
        return make_quat(cangle, -dot(side,xfwd), nup) * qup;
    else
        return make_quat(xfwd, fwd) * qup;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline detail::tquat<T> make_quat_ypr( const detail::tvec3<T>& ypr )
{
	detail::tvec3<T> c = glm::cos(ypr * T(0.5));
	detail::tvec3<T> s = glm::sin(ypr * T(0.5));

    //http://graphics.wikia.com/wiki/Conversion_between_quaternions_and_Euler_angles
	return detail::tquat<T>(
        c.x*c.y*c.z + s.x*s.y*s.z,
        c.x*s.y*c.z - s.x*c.y*s.z,
        s.x*c.y*c.z + c.x*s.y*s.z,
        c.x*c.y*s.z - s.x*s.y*c.z);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline __m128 cross_3shuffles(__m128 a, __m128 b)
{
    const __m128 a_yzx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
    const __m128 b_yzx = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
    const __m128 c = _mm_sub_ps(_mm_mul_ps(a, b_yzx), _mm_mul_ps(a_yzx, b));
    return _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1));
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline __m128 _mm_cross4_ps(__m128 xyzw, __m128 abcd)
{
    /* The product of two quaternions is:                                 */
    /* (X,Y,Z,W) = (xd+yc-zb+wa, -xc+yd+za+wb, xb-ya+zd+wc, -xa-yb-zc+wd) */

    __m128 wzyx = _mm_shuffle_ps(xyzw, xyzw, _MM_SHUFFLE(0, 1, 2, 3));
    __m128 baba = _mm_shuffle_ps(abcd, abcd, _MM_SHUFFLE(0, 1, 0, 1));
    __m128 dcdc = _mm_shuffle_ps(abcd, abcd, _MM_SHUFFLE(2, 3, 2, 3));

    /* variable names below are for parts of componens of result (X,Y,Z,W) */
    /* nX stands for -X and similarly for the other components             */

    /* znxwy  = (xb - ya, zb - wa, wd - zc, yd - xc) */
    __m128 ZnXWY = _mm_hsub_ps(_mm_mul_ps(xyzw, baba), _mm_mul_ps(wzyx, dcdc));

    /* xzynw  = (xd + yc, zd + wc, wb + za, yb + xa) */
    __m128 XZYnW = _mm_hadd_ps(_mm_mul_ps(xyzw, dcdc), _mm_mul_ps(wzyx, baba));

    /* _mm_shuffle_ps(XZYnW, ZnXWY, _MM_SHUFFLE(3,2,1,0)) */
    /*      = (xd + yc, zd + wc, wd - zc, yd - xc)        */
    /* _mm_shuffle_ps(ZnXWY, XZYnW, _MM_SHUFFLE(2,3,0,1)) */
    /*      = (zb - wa, xb - ya, yb + xa, wb + za)        */

    /* _mm_addsub_ps adds elements 1 and 3 and subtracts elements 0 and 2, so we get: */
    /* _mm_addsub_ps(*, *) = (xd+yc-zb+wa, xb-ya+zd+wc, wd-zc+yb+xa, yd-xc+wb+za)     */

    __m128 XZWY = _mm_addsub_ps(_mm_shuffle_ps(XZYnW, ZnXWY, _MM_SHUFFLE(3, 2, 1, 0)),
        _mm_shuffle_ps(ZnXWY, XZYnW, _MM_SHUFFLE(2, 3, 0, 1)));

    /* now we only need to shuffle the components in place and return the result      */
    return _mm_shuffle_ps(XZWY, XZWY, _MM_SHUFFLE(2, 1, 3, 0));

    /* operations: 6 shuffles, 4 multiplications, 3 compound additions/subtractions   */
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static const __m128 _ps_minus_cephes_DP1 = _mm_set1_ps(-0.78515625f);
static const __m128 _ps_minus_cephes_DP2 = _mm_set1_ps(-2.4187564849853515625e-4f);
static const __m128 _ps_minus_cephes_DP3 = _mm_set1_ps(-3.77489497744594108e-8f);
static const __m128 _ps_sincof_p0 = _mm_set1_ps(-1.9515295891E-4f);
static const __m128 _ps_sincof_p1 = _mm_set1_ps(8.3321608736E-3f);
static const __m128 _ps_sincof_p2 = _mm_set1_ps(-1.6666654611E-1f);
static const __m128 _ps_coscof_p0 = _mm_set1_ps(2.443315711809948E-005f);
static const __m128 _ps_coscof_p1 = _mm_set1_ps(-1.388731625493765E-003f);
static const __m128 _ps_coscof_p2 = _mm_set1_ps(4.166664568298827E-002f);
static const __m128 _ps_cephes_FOPI = _mm_set1_ps(1.27323954473516f); // 4 / M_PI

static const __m128 _ps_sign_mask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
static const __m128 _ps_inv_sign_mask = _mm_castsi128_ps(_mm_set1_epi32(~0x80000000));

static const __m128 _ps_1 = _mm_set1_ps(1);
static const __m128 _ps_0p5 = _mm_set1_ps(.5f);

static const __m128i _pi32_1 = _mm_set1_epi32(1);
static const __m128i _pi32_inv1 = _mm_set1_epi32(~1);
static const __m128i _pi32_2 = _mm_set1_epi32(2);
static const __m128i _pi32_4 = _mm_set1_epi32(4);

inline __m128 _mm_sincos_ps(__m128 * mem_addr, __m128 a)
{
    __m128 xmm1, xmm2, xmm3 = _mm_setzero_ps(), sign_bit_sin, y;
    __m128i emm0, emm2, emm4;
    sign_bit_sin = a;
    // take the absolute value
    a = _mm_and_ps(a, _ps_inv_sign_mask);
    // extract the sign bit (upper one)
    sign_bit_sin = _mm_and_ps(sign_bit_sin, _ps_sign_mask);

    // scale by 4/Pi
    y = _mm_mul_ps(a, _ps_cephes_FOPI);

    // store the integer part of y in emm2
    emm2 = _mm_cvttps_epi32(y);

    // j=(j+1) & (~1) (see the cephes sources)
    emm2 = _mm_add_epi32(emm2, _pi32_1);
    emm2 = _mm_and_si128(emm2, _pi32_inv1);
    y = _mm_cvtepi32_ps(emm2);

    emm4 = emm2;

    // get the swap sign flag for the sine
    emm0 = _mm_and_si128(emm2, _pi32_4);
    emm0 = _mm_slli_epi32(emm0, 29);
    __m128 swap_sign_bit_sin = _mm_castsi128_ps(emm0);

    // get the polynom selection mask for the sine
    emm2 = _mm_and_si128(emm2, _pi32_2);
    emm2 = _mm_cmpeq_epi32(emm2, _mm_setzero_si128());
    __m128 poly_mask = _mm_castsi128_ps(emm2);

    // The magic pass: "Extended precision modular arithmetic"
    // x = ((x - y * DP1) - y * DP2) - y * DP3;
    xmm1 = _ps_minus_cephes_DP1;
    xmm2 = _ps_minus_cephes_DP2;
    xmm3 = _ps_minus_cephes_DP3;
    xmm1 = _mm_mul_ps(y, xmm1);
    xmm2 = _mm_mul_ps(y, xmm2);
    xmm3 = _mm_mul_ps(y, xmm3);
    a = _mm_add_ps(a, xmm1);
    a = _mm_add_ps(a, xmm2);
    a = _mm_add_ps(a, xmm3);

    emm4 = _mm_sub_epi32(emm4, _pi32_2);
    emm4 = _mm_andnot_si128(emm4, _pi32_4);
    emm4 = _mm_slli_epi32(emm4, 29);
    __m128 sign_bit_cos = _mm_castsi128_ps(emm4);

    sign_bit_sin = _mm_xor_ps(sign_bit_sin, swap_sign_bit_sin);


    // Evaluate the first polynom  (0 <= x <= Pi/4)
    __m128 z = _mm_mul_ps(a, a);

    y = _mm_mul_ps(_ps_coscof_p0, z);
    y = _mm_add_ps(y, _ps_coscof_p1);
    y = _mm_mul_ps(y, z);
    y = _mm_add_ps(y, _ps_coscof_p2);
    y = _mm_mul_ps(y, z);
    y = _mm_mul_ps(y, z);
    __m128 tmp = _mm_mul_ps(z, _ps_0p5);
    y = _mm_sub_ps(y, tmp);
    y = _mm_add_ps(y, _ps_1);

    // Evaluate the second polynom  (Pi/4 <= x <= 0)

    __m128 y2 = _mm_mul_ps(_ps_sincof_p0, z);
    y2 = _mm_add_ps(y2, _ps_sincof_p1);
    y2 = _mm_mul_ps(y2, z);
    y2 = _mm_add_ps(y2, _ps_sincof_p2);
    y2 = _mm_mul_ps(y2, z);
    y2 = _mm_mul_ps(y2, a);
    y2 = _mm_add_ps(y2, a);

    // select the correct result from the two polynoms
    xmm3 = poly_mask;
    __m128 ysin2 = _mm_and_ps(xmm3, y2);
    __m128 ysin1 = _mm_andnot_ps(xmm3, y);
    y2 = _mm_sub_ps(y2, ysin2);
    y = _mm_sub_ps(y, ysin1);

    xmm1 = _mm_add_ps(ysin1, ysin2);
    xmm2 = _mm_add_ps(y, y2);

    // update the sign
    *mem_addr = _mm_xor_ps(xmm2, sign_bit_cos);

    return _mm_xor_ps(xmm1, sign_bit_sin);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline __m128 _mm_dot4_ps(__m128 r0, __m128 r1)
{
    // SSE2 
    /*r0 = _mm_mul_ps(r0, r1);
    r1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(2, 3, 0, 1));
    r0 = _mm_add_ps(r0, r1);
    r1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(0, 1, 2, 3));
    r0 = _mm_add_ps(r0, r1);
    return r0;*/

    // SSE3 version
    r0 = _mm_mul_ps(r0, r1);
    r0 = _mm_hadd_ps(r0, r0);
    return _mm_hadd_ps(r0, r0);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline __m128 _mm_dot4_ps(__m128 r0)
{
    // SSE2 
    /*r0 = _mm_mul_ps(r0, r0);
    __m128 r1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(2, 3, 0, 1));
    r0 = _mm_add_ps(r0, r1);
    r1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(0, 1, 2, 3));
    r0 = _mm_add_ps(r0, r1);
    return r0;*/

    // SSE3 version
    r0 = _mm_mul_ps(r0, r0);
    r0 = _mm_hadd_ps(r0, r0);
    return _mm_hadd_ps(r0, r0);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static const __m128 __three = _mm_set_ps1(3.0f);
static const __m128 __half = _mm_set_ps1(-0.5f);
static const __m128 __minus_zero = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));  /// compiler BUG ignoring _mm_set_ps1(-0.f)
static const __m128 __zero = _mm_set_ps1(0.0f);
static const __m128i __zeroi = _mm_set1_epi32(0);
static __m128 __short_n_r = _mm_set_ps1(1.f / 32767.f);

inline __m128 _mm_rlength(__m128 r0)
{
    r0 = glm::_mm_dot4_ps(r0);
    __m128 r1 = _mm_rsqrt_ps(r0);

    // one Newton-Rhapson iteration (approximated _mm_rsqrt_ss IS NOT ENOUGH)
    r0 = _mm_mul_ps(_mm_mul_ps(r0, r1), r1);
    r0 = _mm_mul_ps(_mm_sub_ps(r0, __three), _mm_mul_ps(r1, __half));
    return r0;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline __m128 _mm_cross_3shuffles(__m128 a, __m128 b)
{
    __m128 a_yzx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 b_yzx = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 c = _mm_sub_ps(_mm_mul_ps(a, b_yzx), _mm_mul_ps(a_yzx, b));
    return _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1));
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

__forceinline quat mul_quat_simd(const quat &q0, const quat &q1)
{
    __m128 res = _mm_cross4_ps(
        reinterpret_cast<const __m128&>(q0),
        reinterpret_cast<const __m128&>(q1));
    return reinterpret_cast<const quat&>(res);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

__forceinline quat mul_dquat_simd(
    const quat &rot_l,
    const quat &dq_l,
    const quat &rot_r,
    const quat &dq_r)
{
    const __m128 &q0x = reinterpret_cast<const __m128&>(dq_l);
    const __m128 &q1x = reinterpret_cast<const __m128&>(rot_r);
    const __m128 &q2x = reinterpret_cast<const __m128&>(rot_l);
    const __m128 &q3x = reinterpret_cast<const __m128&>(dq_r);

    __m128 res = _mm_add_ps(_mm_cross4_ps(q0x, q1x), _mm_cross4_ps(q2x, q3x));

    return reinterpret_cast<const quat&>(res);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline detail::tvec3<T> ray_from_screen_pos(
    const detail::tvec2<T> &pos,
    const detail::tvec2<T> &size,
    const detail::tmat4x4<T> &imvp)
{
    const detail::tvec4<T> wpos = imvp * detail::tvec4<T>(
        pos * (T(2.0) / size) - T(1.0), T(-1.0), T(1.0));
	return glm::normalize(detail::tvec3<T>(wpos) * (T(1.0) / wpos.w));
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline bool sphere_intersection(
    const detail::tvec3<T>& pos,
    const detail::tvec3<T>& dir,
    const T R,
    detail::tvec3<T> *out)
{
    const typename detail::tvec3<T>::value_type D2 = dot(pos, pos);
    const typename detail::tvec3<T>::value_type R2 = R * R;
	const typename detail::tvec3<T>::value_type B = T(2.0) * dot(pos, dir);
	const typename detail::tvec3<T>::value_type C = D2 - R2;
	const typename detail::tvec3<T>::value_type fDet = B * B - T(4.0) * C;
    
    if(fDet < T(0.0)) return false;

	*out = pos + T(0.5) * (-B - sqrt(fDet)) * dir;

    return true;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline bool sphere_intersection2(
    const detail::tvec3<T>& pos,
    const detail::tvec3<T>& dir,
    const T R,
    T *t)
{
    const T D2 = dot(pos, pos);
    const T R2 = R * R;
	const T B = T(2.0) * dot(pos, dir);
	const T C = D2 - R2;
	const T fDet = B * B - T(4.0) * C;
    
    if(fDet < T(0.0)) return false;

	*t = T(0.5) * (-B - sqrt(fDet));

    return true;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
bool sphere_intersects_aabb(
    const detail::tvec3<T>& center,
    T radius,
    const detail::tvec3<T>& aabb_half,
    T* dist)
{
    T dmin = 0;
    T r2 = radius * radius;

    for (int i = 0; i < 3; i++) {
		T d = .0;
		if (center[i] > aabb_half[i]) {
			d = center[i] - aabb_half[i];
		}
		else if (center[i] < -aabb_half[i])
		{
			d = center[i] + aabb_half[i];
			
		}
        dmin += d * d;
    }

    if (dmin <= r2) {
        if (dist)
            *dist = sqrt(dmin);

        return true;
    }

    return false;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
bool aabb_intersects_aabb(
    const detail::tvec3<T>& aabb1_center,
    const detail::tvec3<T>& aabb1_half,
    const detail::tvec3<T>& aabb2_half,
    T* dist)
{
    const detail::tvec3<T> d = abs(aabb1_center) - (aabb1_half + aabb2_half);

    if (d.x < 0 && d.y < 0 && d.z < 0) {
        if (dist)
            *dist = -max(d.x,max(d.y,d.z));

        return true;
    }

    return false;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline detail::tquat<T> slerp(
    const detail::tquat<T>& from,
    const detail::tquat<T>& to,
    const T t,
    bool adjustSign = true)
{
    const detail::tquat<T>& p = from; // just an alias to match q

    // calc cosine theta
    T cosom = dot(from, to);

    // adjust signs (if necessary)
    detail::tquat<T> q;
    if(adjustSign && (cosom < (T)0.0)) {
        cosom = -cosom;
        q.x = -to.x;   // Reverse all signs
        q.y = -to.y;
        q.z = -to.z;
        q.w = -to.w;
    }
    else {
        q = to;
    }

    // Calculate coefficients
    T sclp, sclq;
    if(((T)1.0 - cosom) > (T)0.0001) {
        // Standard case (slerp)
        T omega, sinom;
        omega = acos(cosom); // extract theta from dot product's cos theta
        sinom = sin(omega);
        sclp  = sin(((T)1.0 - t) * omega) / sinom;
        sclq  = sin(t * omega) / sinom;
    }
    else {
        // Very close, do linear interp (because it's faster)
        sclp = (T)1.0 - t;
        sclq = t;
    }

    return detail::tquat<T>(
        sclp * p.w + sclq * q.w,
        sclp * p.x + sclq * q.x,
        sclp * p.y + sclq * q.y,
        sclp * p.z + sclq * q.z);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline detail::tquat<T> lerp(
    const detail::tquat<T>& from,
    const detail::tquat<T>& to,
    const T t,
    bool adjustSign = true)
{
   const detail::tquat<T>& p = from; // just an alias to match q

    // calc cosine theta
    T cosom = dot(from, to);

    // adjust signs (if necessary)
    detail::tquat<T> q;
    if(adjustSign && (cosom < T(0.0))) {
        cosom = -cosom;
        q.x = -to.x;   // Reverse all signs
        q.y = -to.y;
        q.z = -to.z;
        q.w = -to.w;
    }
    else {
        q = to;
    }

    // do linear interp
    const T sclp = T(1.0) - t;

    return detail::tquat<T>(
        sclp * p.w + t * q.w,
        sclp * p.x + t * q.x,
        sclp * p.y + t * q.y,
        sclp * p.z + t * q.z);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

///
inline float triangle_area(glm::vec3 *points)
{
	float a=glm::length(points[1]-points[0]);
	float b=glm::length(points[2]-points[1]);
	float c=glm::length(points[2]-points[0]);
	float p=(a+b+c)/2.0f;
	return glm::sqrt(p*(p-a)*(p-b)*(p-c));
}

/*
inline coid::binstream& operator << (coid::binstream& bin, const vec3& s)
{   bin.write_fixed_array_content(value_ptr(s),3); return bin; }

inline coid::binstream& operator >> (coid::binstream& bin, vec3& s)
{   bin.read_fixed_array_content(value_ptr(s),3); return bin; }

inline coid::metastream& operator << (coid::metastream& m, const vec3& s)
{
	MSTRUCT_OPEN(m,"glm::vec3")
		MMAF(m, "XYZ", double,3)
	MSTRUCT_CLOSE(m)
}*/

template<class T>
inline detail::tvec4<T> normalize_plane(const detail::tvec4<T> &plane) {
	return plane * (T(1.0) / length(detail::tvec3<T>(plane)));
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
/*
inline void create_frustum_planes_for_culling(simdVec4 *planes, const mat4x4 &mvp)
{
    // Left plane
    planes[0] = simdVec4(normalize_plane(float4(mvp[0].w+mvp[0].x,mvp[1].w+mvp[1].x,mvp[2].w+mvp[2].x,mvp[3].w+mvp[3].x)));
 
    // Right plane
    planes[1] = simdVec4(normalize_plane(float4(mvp[0].w-mvp[0].x,mvp[1].w-mvp[1].x,mvp[2].w-mvp[2].x,mvp[3].w-mvp[3].x)));
 
    // Top plane
    planes[2] = simdVec4(normalize_plane(float4(mvp[0].w-mvp[0].y,mvp[1].w-mvp[1].y,mvp[2].w-mvp[2].y,mvp[3].w-mvp[3].y)));
 
    // Bottom plane
    planes[3] = simdVec4(normalize_plane(float4(mvp[0].w+mvp[0].y,mvp[1].w+mvp[1].y,mvp[2].w+mvp[2].y,mvp[3].w+mvp[3].y)));
 
    // Far plane
    planes[4] = simdVec4(normalize_plane(float4(mvp[0].w-mvp[0].z,mvp[1].w-mvp[1].z,mvp[2].w-mvp[2].z,mvp[3].w-mvp[3].z)));

	// Near plane
    planes[5] = simdVec4(normalize_plane(float4(mvp[0].z,mvp[1].z,mvp[2].z,mvp[3].z)));
}
*/
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline void create_frustum_planes(vec4 *planes, const mat4 &mvp)
{
    // Left plane
    planes[1]=normalize_plane(vec4(mvp[0].w+mvp[0].x,mvp[1].w+mvp[1].x,mvp[2].w+mvp[2].x,mvp[3].w+mvp[3].x));
	//planes[0]=vec4(abs(planes[1].swizzle(X,Y,Z)),planes[1].w);
    planes[0]=vec4(fabs(planes[1].x), fabs(planes[1].y), fabs(planes[1].z), planes[1].w);
 
    // Right plane
    planes[3]=normalize_plane(vec4(mvp[0].w-mvp[0].x,mvp[1].w-mvp[1].x,mvp[2].w-mvp[2].x,mvp[3].w-mvp[3].x));
	planes[2]=vec4(fabs(planes[3].x), fabs(planes[3].y), fabs(planes[3].z), planes[3].w);
 
    // Top plane
    planes[5]=normalize_plane(vec4(mvp[0].w-mvp[0].y,mvp[1].w-mvp[1].y,mvp[2].w-mvp[2].y,mvp[3].w-mvp[3].y));
	planes[4]=vec4(fabs(planes[5].x), fabs(planes[5].y), fabs(planes[5].z), planes[5].w);
 
    // Bottom plane
    planes[7]=normalize_plane(vec4(mvp[0].w+mvp[0].y,mvp[1].w+mvp[1].y,mvp[2].w+mvp[2].y,mvp[3].w+mvp[3].y));
	planes[6]=vec4(fabs(planes[7].x), fabs(planes[7].y), fabs(planes[7].z), planes[7].w);
 
    // Far plane
    planes[9]=normalize_plane(vec4(mvp[0].w-mvp[0].z,mvp[1].w-mvp[1].z,mvp[2].w-mvp[2].z,mvp[3].w-mvp[3].z));
	planes[8]=vec4(fabs(planes[9].x), fabs(planes[9].y), fabs(planes[9].z), planes[11].w);

	// Near plane
    planes[11]=normalize_plane(vec4(mvp[0].z,mvp[1].z,mvp[2].z,mvp[3].z));
	planes[10]=vec4(fabs(planes[11].x), fabs(planes[11].y), fabs(planes[11].z), planes[9].w);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline void create_frustum_planes_for_culling(float4 *planes, const float4x4 &mvp)
{
    planes[0] = glm::normalize_plane(float4(mvp[0].w+mvp[0].x, mvp[1].w+mvp[1].x, mvp[2].w+mvp[2].x, mvp[3].w+mvp[3].x));
    planes[1] = glm::normalize_plane(float4(mvp[0].w-mvp[0].x, mvp[1].w-mvp[1].x, mvp[2].w-mvp[2].x, mvp[3].w-mvp[3].x));
    planes[2] = glm::normalize_plane(float4(mvp[0].w-mvp[0].y, mvp[1].w-mvp[1].y, mvp[2].w-mvp[2].y, mvp[3].w-mvp[3].y));
    planes[3] = glm::normalize_plane(float4(mvp[0].w+mvp[0].y, mvp[1].w+mvp[1].y, mvp[2].w+mvp[2].y, mvp[3].w+mvp[3].y));
    planes[4] = glm::normalize_plane(float4(mvp[0].w-mvp[0].z, mvp[1].w-mvp[1].z, mvp[2].w-mvp[2].z, mvp[3].w-mvp[3].z));
    planes[5] = glm::normalize_plane(float4(mvp[0].z,mvp[1].z, mvp[2].z,mvp[3].z));
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class T>
inline bool calc_tangent_bitangent(
    const detail::tvec3<T> &v0,
    const detail::tvec3<T> &v1,
    const detail::tvec3<T> &v2,
    const detail::tvec2<T> &uv0,
    const detail::tvec2<T> &uv1,
    const detail::tvec2<T> &uv2,
	detail::tvec3<T>& tangent,
    detail::tvec3<T>& bitangent)
{
    const detail::tvec3<T> deltaPos1 = v1 - v0;
    const detail::tvec3<T> deltaPos2 = v2 - v0;
    const detail::tvec2<T> deltaUV1 = uv1 - uv0;
    const detail::tvec2<T> deltaUV2 = uv2 - uv0;

    const T d = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;

    if(abs(d) < T(0.0000001))
        return false;

    const T r = T(1.0) / d;

    tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
    bitangent = -(deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

    T len = glm::length(tangent);
    if(len > 0)
        tangent /= len;

    len = glm::length(bitangent);
    if(len > 0)
        bitangent /= len;

    return true;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class T>
inline bool calc_tangent_mitring(
    const detail::tvec3<T> &v0,
    const detail::tvec3<T> &v1,
    const detail::tvec3<T> &v2,
    const detail::tvec2<T> &uv0,
    const detail::tvec2<T> &uv1,
    const detail::tvec2<T> &uv2,
    detail::tvec3<T>& tangent,
    detail::tvec3<T>& bitangent,
    detail::tvec3<T>& normal)
{
    const detail::tvec3<T> vA = v1 - v0;
    const detail::tvec3<T> vB = v2 - v0;

    float fDeltaU1 = uv1.x - uv0.x;
    float fDeltaU2 = uv2.x - uv0.x;
    float fDeltaV1 = uv1.y - uv0.y;
    float fDeltaV2 = uv2.y - uv0.y;
     
    float div = (fDeltaU1 * fDeltaV2 - fDeltaU2 * fDeltaV1);

    bool valid = true;

    if(_isnan(div)) {
        div = 0.0f;
    }

    detail::tvec3<T> vU, vV, vN = glm::normalize(cross(vA, vB));

    if(div != 0.0f) {
        //	2D triangle area = (u1*v2-u2*v1)/2
        // weight the tangent vectors by the UV triangles area size (fix problems with base UV assignment)
        float fAreaMul2 = glm::abs(fDeltaU1 * fDeltaV2 - fDeltaU2 * fDeltaV1);

        float a = fDeltaV2 / div;
        float b = -fDeltaV1 / div;
        float c = -fDeltaU2 / div;
        float d = fDeltaU1 / div;

        vU = normalize(vA * a + vB * b) * fAreaMul2;
        vV = normalize(vA * c + vB * d) * fAreaMul2;
    }
    else {
        vU = detail::tvec3<T>(1, 0, 0); vV = detail::tvec3<T>(0, 1, 0);
        valid = false;
    }

    tangent = vU;
    bitangent = vV;
    normal = vN;
    
    return valid;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline detail::tquat<float> to_quat( const detail::tquat<double> &rot )
{
    return detail::tquat<float>(float(rot.w), float(rot.x), float(rot.y), float(rot.z));
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline detail::tquat<double> to_dquat( const detail::tquat<float> &rot )
{
    return detail::tquat<double>(rot.w, rot.x, rot.y, rot.z);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline detail::tquat<T> to_dquat(
    const detail::tquat<T> &rot,
    const detail::tvec3<T> &trans)
{
    return detail::tquat<T>(
       T(-0.5) * (trans.x * rot.x + trans.y * rot.y + trans.z * rot.z),
       T(0.5) * ( trans.x * rot.w + trans.y * rot.z - trans.z * rot.y),
       T(0.5) * (-trans.x * rot.z + trans.y * rot.w + trans.z * rot.x),
       T(0.5) * ( trans.x * rot.y - trans.y * rot.x + trans.z * rot.w));
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline detail::tvec3<T> dquat_to_trans(
    const detail::tquat<T> &rot,
    const detail::tquat<T> &dq)
{
    return float3(
        2.0f * (-dq.w * rot.x + dq.x * rot.w - dq.y * rot.z + dq.z * rot.y),
        2.0f * (-dq.w * rot.y + dq.x * rot.z + dq.y * rot.w - dq.z * rot.x),
        2.0f * (-dq.w * rot.z - dq.x * rot.y + dq.y * rot.x + dq.z * rot.w));
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline detail::tvec3<T> dquat_to_trans_xy(
    const detail::tquat<T> &rot,
    const detail::tquat<T> &dq)
{
    return float3(
        2.0f * (-dq.w * rot.x + dq.x * rot.w - dq.y * rot.z + dq.z * rot.y),
        2.0f * (-dq.w * rot.y + dq.x * rot.z + dq.y * rot.w - dq.z * rot.x),
        0);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline detail::tquat<T> mult_dquat(
    const detail::tquat<T> &rot_l,
    const detail::tquat<T> &dq_l,
    const detail::tquat<T> &rot_r,
    const detail::tquat<T> &dq_r)
{
    return dq_l * rot_r + rot_l * dq_r;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline const detail::tvec4<T>& to_vec(const detail::tquat<T> &q)
{
    return *reinterpret_cast<const detail::tvec4<T>*>(&q);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline __m128& to_m128(quat &q)
{
    return *reinterpret_cast<__m128*>(&q);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const __m128& to_m128(const quat &q)
{
    return *reinterpret_cast<const __m128*>(&q);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class T>
bool intersect_aab(
    const detail::tvec3<T> &mi,
    const detail::tvec3<T> &ma,
    const detail::tvec3<T> &pos,
    const detail::tvec3<T> &dir,
    T &tIn,
    T &tOut)
{
    tIn  = -FLT_MAX;
    tOut = FLT_MAX;
    T t0, t1;
    const T epsilon(0.0000001);

    // YZ plane.
    if(abs(dir.x) < epsilon)
        if(pos.x < mi.x || pos.x > ma.x)
            return false; // Ray parallel to plane.

    // XZ plane.
    if(abs(dir.y) < epsilon)
        if (pos.y < mi.y || pos.y > ma.y)
            return false; // Ray parallel to plane.

    // XY plane.
    if(abs(dir.z) < epsilon)
        if (pos.z < mi.z || pos.z > ma.z)
            return false; // Ray parallel to plane.

    // YZ plane.
    t0 = (mi.x - pos.x) / dir.x;
    t1 = (ma.x - pos.x) / dir.x;

    if(t0 > t1)
        std::swap(t0, t1);

    if(t0 > tIn)
        tIn = t0;
    if(t1 < tOut)
        tOut = t1;

    if(tIn > tOut || tOut < T(0))
        return false;

    // XZ plane.
    t0 = (mi.y - pos.y) / dir.y;
    t1 = (ma.y - pos.y) / dir.y;

    if(t0 > t1)
        std::swap(t0, t1);

    if(t0 > tIn)
        tIn = t0;
    if(t1 < tOut)
        tOut = t1;

    if(tIn > tOut || tOut < T(0))
        return false;

    // XY plane.
    t0 = (mi.z - pos.z) / dir.z;
    t1 = (ma.z - pos.z) / dir.z;

    if(t0 > t1)
        std::swap(t0, t1);

    if(t0 > tIn)
        tIn = t0;
    if(t1 < tOut)
        tOut = t1;

    if(tIn > tOut || tOut < T(0))
        return false;

    return true;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//@return intersection distance from pos, 0 if inside box, -1 if not intersecting
inline float aabb_ray_test( const float3& minb, const float3& maxb, const float3& pos, const float3& dir, float maxlen )
{
    float3 mx = float3(-1);
    bool inside = true;

    //candidate planes
    for(int i=0; i<3; ++i)
    {
        if(pos[i] < minb[i])
        {
            inside = false;

            // calculate T distances to candidate planes
            if(dir[i]) mx[i] = (minb[i] - pos[i]) / dir[i];
        }
        else if(pos[i] > maxb[i])
        {
            inside = false;

            // calculate T distances to candidate planes
            if(dir[i]) mx[i] = (maxb[i] - pos[i]) / dir[i];
        }
    }

    // ray origin inside bounding box
    if(inside)
        return 0;


    // get largest of the mx for final choice of intersection
    int plane = 0;
    if(mx[1] > mx[plane]) plane = 1;
    if(mx[2] > mx[plane]) plane = 2;

    // check final candidate actually inside box
    if(mx[plane] < 0 || mx[plane] > maxlen)
        return -1;

    for(int i=0; i<3; ++i)
    {
        if(i != plane)
        {
            float v = pos[i] + mx[plane] * dir[i];

            if(v < minb[i] || v > maxb[i])
                return -1;
        }
    }

    return mx[plane];
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class T>
detail::tmat4x4<T> create_crop_matrix(
    const detail::tvec3<T> &mi, const detail::tvec3<T> &ma)
{
    const detail::tvec3<T> scale = detail::tvec3<T>(T(2.0)) / (ma - mi);
    const detail::tvec3<T> offset = T(-0.5) * (ma + mi) * scale;

    return detail::tmat4x4<T>(
        float4(scale.x,0,0,0),
        float4(0,scale.y,0,0),
        float4(0,0,scale.z,0),
        float4(offset,T(1)));
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class T>
void find_bounds(
    const detail::tvec3<T> *points,
    const uint count,
    const detail::tvec3<T> &offset,
    const detail::tmat4x4<T> &tm,
    detail::tvec3<T> &mi,
    detail::tvec3<T> &ma)
{
    auto e = points + count;
    for(auto i = points; i != e; ++i) {
	    detail::tvec4<T> tp = tm * detail::tvec4<T>(*i + offset, 1);
	    detail::tvec3<T> sp = detail::tvec3<T>(tp) / tp.w;
        mi = min(sp, mi);
        ma = max(sp, ma);
    }
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class T>
void find_bounds(
    const detail::tvec3<T> *points,
    const uint count,
    const detail::tmat4x4<T> &tm,
    detail::tvec3<T> &mi,
    detail::tvec3<T> &ma)
{
    auto e = points + count;
    for(auto i = points; i != e; ++i) {
	    detail::tvec4<T> tp = tm * detail::tvec4<T>(*i, 1);
	    detail::tvec3<T> sp = detail::tvec3<T>(tp) / tp.w;
        mi = min(sp, mi);
        ma = max(sp, ma);
    }
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class T>
detail::tvec4<T> create_plane(
    const detail::tvec3<T>& normal,
    const detail::tvec3<T>& point)
{
    return detail::tvec4<T>(normal, dot(normal, point));
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<class T>
detail::tvec4<T> create_plane_nor(
    const detail::tvec3<T>& normal,
    const detail::tvec3<T>& point)
{
    return normalize_plane(create_plane(normal, point));
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Copyright 2001, softSurfer (www.softsurfer.com)
// This code may be freely used and modified for any purpose
// providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.

// Assume that a class is already given for the object:
//    Point with coordinates {float x, y;}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// isLeft(): tests if a point is Left|On|Right of an infinite line.
//    Input:  three points P0, P1, and P2
//    Return: >0 for P2 left of the line through P0 and P1
//            =0 for P2 on the line
//            <0 for P2 right of the line
//    See: the January 2001 Algorithm on Area of Triangles
template<class T>
float chain_hull_is_left(const T &p0, const T &p1, const T &p2)
{
    return (p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// chainHull_2D(): Andrew's monotone chain 2D convex hull algorithm
//     Input:  P[] = an array of 2D points 
//                   presorted by increasing x- and y-coordinates
//             n = the number of points in P[]
//     Output: H[] = an array of the convex hull vertices (max is n)
//     Return: the number of points in H[]
template<class T, class U>
int chain_hull_2D(const T *P, const int n, U *H)
{
    // the output array H[] will be used as the stack
    int bot = 0;
    int top = -1;  // indices for bottom and top of the stack
    int i;

    // Get the indices of points with min x-coord and min|max y-coord
    int minmin = 0, minmax;
    float xmin = P[0].x;
    
    for(i = 1; i<n; ++i)
        if(P[i].x != xmin)
            break;

    minmax = i - 1;
    if(minmax == n - 1) {       // degenerate case: all x-coords == xmin
        H[++top] = P[minmin];
        if(P[minmax].y != P[minmin].y) // a nontrivial segment
            H[++top] = P[minmax];
        H[++top] = P[minmin];           // add polygon endpoint
        return top + 1;
    }

    // Get the indices of points with max x-coord and min|max y-coord
    int maxmin, maxmax = n - 1;
    float xmax = P[n - 1].x;
    for(i = n - 2; i >= 0; --i)
        if(P[i].x != xmax)
            break;
    maxmin = i + 1;

    // Compute the lower hull on the stack H
    H[++top] = P[minmin];      // push minmin point onto stack
    i = minmax;
    while(++i <= maxmin) {
        // the lower line joins P[minmin] with P[maxmin]
        if(chain_hull_is_left(P[minmin], P[maxmin], P[i]) >= 0 && i < maxmin)
            continue;          // ignore P[i] above or on the lower line

        while(top > 0) {       // there are at least 2 points on the stack
            // test if P[i] is left of the line at the stack top
            if(chain_hull_is_left(H[top-1], H[top], P[i]) > 0)
                break;         // P[i] is a new hull vertex
            else
                top--;         // pop top point off stack
        }
        H[++top] = P[i];       // push P[i] onto stack
    }

    // Next, compute the upper hull on the stack H above the bottom hull
    if(maxmax != maxmin)      // if distinct xmax points
        H[++top] = P[maxmax];  // push maxmax point onto stack
    bot = top;                 // the bottom point of the upper hull stack
    i = maxmin;
    while(--i >= minmax) {
        // the upper line joins P[maxmax] with P[minmax]
        if(chain_hull_is_left(P[maxmax], P[minmax], P[i]) >= 0 && i > minmax)
            continue;          // ignore P[i] below or on the upper line

        while(top > bot) {    // at least 2 points on the upper stack
            // test if P[i] is left of the line at the stack top
            if(chain_hull_is_left(H[top-1], H[top], P[i]) > 0)
                break;         // P[i] is a new hull vertex
            else
                top--;         // pop top point off stack
        }
        H[++top] = P[i];       // push P[i] onto stack
    }
    if(minmax != minmin)
        H[++top] = P[minmin];  // push joining endpoint onto stack

    return top + 1;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template <typename T> 
inline detail::tmat4x4<T> rotate
(
	detail::tmat4x4<T> const & m,
    T const & cos_angle, 
	T const & sin_angle, 
	detail::tvec3<T> const & v
)
{
	T c = cos_angle;
	T s = sin_angle;

	detail::tvec3<T> axis = normalize(v);

	detail::tvec3<T> temp = (T(1) - c) * axis;

	detail::tmat4x4<T> Rotate(detail::tmat4x4<T>::null);
	Rotate[0][0] = c + temp[0] * axis[0];
	Rotate[0][1] = 0 + temp[0] * axis[1] + s * axis[2];
	Rotate[0][2] = 0 + temp[0] * axis[2] - s * axis[1];

	Rotate[1][0] = 0 + temp[1] * axis[0] - s * axis[2];
	Rotate[1][1] = c + temp[1] * axis[1];
	Rotate[1][2] = 0 + temp[1] * axis[2] + s * axis[0];

	Rotate[2][0] = 0 + temp[2] * axis[0] + s * axis[1];
	Rotate[2][1] = 0 + temp[2] * axis[1] - s * axis[0];
	Rotate[2][2] = c + temp[2] * axis[2];

	detail::tmat4x4<T> Result(detail::tmat4x4<T>::null);
	Result[0] = m[0] * Rotate[0][0] + m[1] * Rotate[0][1] + m[2] * Rotate[0][2];
	Result[1] = m[0] * Rotate[1][0] + m[1] * Rotate[1][1] + m[2] * Rotate[1][2];
	Result[2] = m[0] * Rotate[2][0] + m[1] * Rotate[2][1] + m[2] * Rotate[2][2];
	Result[3] = m[3];
	return Result;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

///
inline mat4x4 make_rot(const vec3 &from, const vec3 &to) {
	const float cosangle=dot(from,to);

	if(!is_equal(cosangle, 1.0f, 0.00001f))
		return rotate(mat4x4(1.0f), cosangle, sqrtc(1 - cosangle*cosangle), normalize(cross(from,to)));
	else 
		return mat4x4(1.0f);	
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline detail::tmat3x3<T> mat_polar_decompose_rot(const detail::tmat3x3<T> &m)
{
    detail::tmat3x3<T> rot[2];
    float3 err(0);
    uint counter = 0;
    detail::tmat3x3<T> *dst, *src;
        
    rot[0] = m;

    do {
        src = rot + (counter & 1);
        dst = rot + (counter + 1 & 1);
        *dst = (glm::inverse(glm::transpose(*src)) + *src) * 0.5f;
        err = max(
            max(abs((*dst)[0] - (*src)[0]), abs((*dst)[1] - (*src)[1])),
            abs((*dst)[2] - (*src)[2]));
    } while(max(max(err.x, err.y), err.z) > 1e-7f && counter++ < 128);

    return *dst;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template <typename T> 
inline void mat4_build(
    const detail::tquat<T> & q,
    const detail::tvec3<T> & pos,
    detail::tmat4x4<T> & dst)
{
	dst[0][0] = 1 - 2 * q.y * q.y - 2 * q.z * q.z;
	dst[0][1] = 2 * q.x * q.y + 2 * q.w * q.z;
	dst[0][2] = 2 * q.x * q.z - 2 * q.w * q.y;
	dst[0][3] = 0;

	dst[1][0] = 2 * q.x * q.y - 2 * q.w * q.z;
	dst[1][1] = 1 - 2 * q.x * q.x - 2 * q.z * q.z;
	dst[1][2] = 2 * q.y * q.z + 2 * q.w * q.x;
	dst[1][3] = 0;

	dst[2][0] = 2 * q.x * q.z + 2 * q.w * q.y;
	dst[2][1] = 2 * q.y * q.z - 2 * q.w * q.x;
	dst[2][2] = 1 - 2 * q.x * q.x - 2 * q.y * q.y;
	dst[2][3] = 0;

	dst[3][0] = pos.x;
	dst[3][1] = pos.y;
	dst[3][2] = pos.z;
	dst[3][3] = 1;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline void slerp(
    detail::tquat<T> &dst,
    detail::tquat<T> &dst_d,
    const detail::tquat<T>& from,
    const detail::tquat<T>& from_d,
    const detail::tquat<T>& to,
    const detail::tquat<T>& to_d,
    const T t)
{
    // Shortest path
    const T d = dot(from, to);

    const detail::tquat<T> q = d < T(0) ? to * T(-1) : to;
    const detail::tquat<T> qd = d < T(0) ? to_d * T(-1) : to_d;
    
    // ScLERP = qa(qa^-1 qb)^t
    detail::tquat<T> diff = q * conjugate(from);
    detail::tquat<T> diff_d = mult_dquat(q, qd, conjugate(from), conjugate(from_d));

    detail::tvec3<T> vr = detail::tvec3<T>(diff.x, diff.y, diff.z);
    detail::tvec3<T> vd = detail::tvec3<T>(diff_d.x, diff_d.y, diff_d.z);
    const T dot_vr = dot(vr, vr);

    if(dot_vr == 0.0f/* T(0.0000001)*/) {
        dst = to;
        dst_d = to_d;
        return;
    }

    const T invr = T(1) / sqrt(dot_vr);
    
    // Screw parameters
    T angle = T(2) * acos(diff.w);
    T pitch = T(-2) * diff_d.w * invr;
    
    detail::tvec3<T> direction =  vr * invr;
    detail::tvec3<T> moment =  (vd - direction * pitch * diff.w * T(0.5)) * invr;
  
    // Exponential power
    angle = angle * t * T(0.5);
    pitch *= t;
    
    // Convert back to dual-quaternion
    const T sinAngle = sin(angle);
    const T cosAngle = cos(angle);
    
    detail::tquat<T> real(cosAngle, direction * sinAngle);
    detail::tquat<T> dual(
        -pitch * T(0.5) * sinAngle,
        sinAngle * moment + pitch * T(0.5) * cosAngle * direction);

    // Complete the multiplication and return the interpolated value
    dst = real * from;
    dst_d = mult_dquat(real, dual, from, from_d);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/*
         v 
    B o-----o C
      | o X |
     u|     |
      |     |
    A o-----o D

*/
template<typename T>
inline bool inverse_bilinear(
    const detail::tvec2<T> &A,
    const detail::tvec2<T> &B,
    const detail::tvec2<T> &C,
    const detail::tvec2<T> &D,
    const detail::tvec2<T> &X,
    detail::tvec2<T> &uv)
{
    const detail::tvec2<T> E = B - A;
    const detail::tvec2<T> F = D - A;
    const detail::tvec2<T> G = A - B + C - D;
    const detail::tvec2<T> H = X - A;

    const T k2 = G.x * F.y - G.y * F.x;
    const T k1 = E.x * F.y - E.y * F.x + H.x * G.y - H.y * G.x;
    const T k0 = H.x * E.y - H.y * E.x;
    const T d = k1 * k1 - 4.f * k0 * k2;

    if(d < T(0))
        return false;

    if(k2 != T(0)) {
        const float sq = glm::sqrt(d);
        const float c = T(2.0) * k2;
        const float v0 = (-k1 + sq) / c;
        const float v1 = (-k1 - sq) / c;
        uv.y = v0 >= T(0) && v0 <= T(1.0) ? v0 : v1;
    }
    else {
        uv.y = -k0 / k1;
    }

    uv.x = (H.x - F.x * uv.y) / (E.x + G.x * uv.y);

    return true;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline detail::tvec2<T> barycentric(
    const detail::tvec2<T> &A,
    const detail::tvec2<T> &B,
    const detail::tvec2<T> &C,
    const detail::tvec2<T> &X)
{
    const detail::tvec2<T> v0 = C - A;
    const detail::tvec2<T> v1 = B - A;
    const detail::tvec2<T> v2 = X - A;

    const T dot00 = dot(v0, v0);
    const T dot01 = dot(v0, v1);
    const T dot02 = dot(v0, v2);
    const T dot11 = dot(v1, v1);
    const T dot12 = dot(v1, v2);

    // Compute barycentric coordinates
    const T denom = dot00 * dot11 - dot01 * dot01;
    if (denom == T(0)) {
        return detail::tvec2<T>(-1);
    }
    else {
        const T invDenom = T(1.0) / (dot00 * dot11 - dot01 * dot01);

        return detail::tvec2<T>(
            (dot11 * dot02 - dot01 * dot12) * invDenom,
            (dot00 * dot12 - dot01 * dot02) * invDenom);
    }
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline glm::detail::tquat<T> to_model_rot( const detail::tquat<T>& camrot )
{
    return camrot * detail::tquat<T>(-M_SQRT1_2, M_SQRT1_2, 0, 0);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline glm::detail::tquat<T> to_camera_rot(const detail::tquat<T>& modrot)
{
    return detail::tquat<T>(M_SQRT1_2, M_SQRT1_2, 0, 0) * modrot;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline glm::detail::tquat<T> quat_from_ypr(
    const detail::tvec3<T>& ypr,
    const bool camera = false)
{
    glm::detail::tquat<T> qy = glm::make_quat_y(-ypr.x);// (-ypr.x, detail::tvec3<T>(0, 1, 0));

    const detail::tvec3<T> xright = mul_quat_x(qy);// * detail::tvec3<T>(1, 0, 0);

    glm::detail::tquat<T> qp = glm::make_quat(ypr.y, xright) * qy;

    const detail::tvec3<T> xfwd = -mul_quat_z(qp);// * detail::tvec3<T>(0, 0, -1);

    glm::detail::tquat<T> qr = glm::make_quat(ypr.z, xfwd) * qp;

    if(!camera)
        qr = to_model_rot(qr);

    return qr;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline detail::tvec3<T> ypr_from_quat(
    const detail::tquat<T>& q,
    const bool camera = false)
{
    detail::tquat<T> qm;
    const detail::tquat<T>& qs = camera
        ? q
        : (qm = to_camera_rot(q));

    //const detail::tvec3<T> right = detail::tvec3<T>(1, 0, 0);
    //const detail::tvec3<T> fwd = detail::tvec3<T>(0, 0, -1);
    const detail::tvec3<T> up = detail::tvec3<T>(0, 1, 0);

    const detail::tvec3<T> dir = -mul_quat_z(qs);
    const detail::tvec3<T> cup = mul_quat_y(qs);

    const float pitch = asin(glm::clamp(dir.y, -1.0f, 1.0f));

    bool ok = fabs(dir.y) < T(1.0) - T(0.00005);
    float yaw = ok
        ? atan2(dir.x, -dir.z)
        : atan2(-cup.x, cup.z);

    T roll = 0;
    if(ok) {
        float3 horz = ok
            ? glm::normalize(glm::cross(up, dir))
            : mul_quat_x(qs);

        const detail::tvec3<T> rup = glm::cross(horz, dir);

        roll = -atan2(glm::dot(horz, cup), -glm::dot(rup, cup));
    }

    return detail::tvec3<T>(yaw, pitch, roll);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline quat quat_from_hpr(
    const detail::tvec3<T> &up,
    const detail::tvec3<T> &hpr,
    const bool camera = false)
{
    glm::detail::tquat<T> qu = glm::make_quat_zy_align_fwd(detail::tvec3<T>(0, 0, 1), up);

    glm::detail::tquat<T> qh = glm::make_quat(-hpr.x, up) * qu;

    const detail::tvec3<T> xwest = -mul_quat_x(qh);// * detail::tvec3<T>(-1, 0, 0);

    glm::detail::tquat<T> qp = glm::make_quat(-hpr.y, xwest) * qh;

    const detail::tvec3<T> xfwd = -mul_quat_z(qp);// * detail::tvec3<T>(0, 0, -1);

    glm::detail::tquat<T> qr = glm::make_quat(hpr.z, xfwd) * qp;

    if(!camera)
        qr = to_model_rot(qr);

    return qr;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline detail::tvec3<T> hpr_from_quat(
    const detail::tvec3<T> &up,
    const glm::detail::tquat<T> &qs,
    const bool camera = false)
{
    const detail::tvec3<T> west = glm::normalize(float3(up.y, -up.x, 0)); //cross(up, float3(0,0,1));
    const detail::tvec3<T> north = glm::cross(west, up);

    const detail::tvec3<T> dir = qs * (camera ? detail::tvec3<T>(0, 0, -1) : detail::tvec3<T>(0, 1, 0));
    const detail::tvec3<T> cup = qs * (camera ? detail::tvec3<T>(0, 1, 0) : detail::tvec3<T>(0, 0, 1));

    const T z = glm::dot(north, dir);
    const T x = glm::dot(west, dir);
    const T y = glm::dot(up, dir);

    const float pitch = asin(glm::clamp(y, -1.0f, 1.0f));

    bool ok = fabs(y) < T(1.0) - T(0.00005);
    float heading = ok
        ? atan2(-x, z)
        : atan2(glm::dot(west, cup), -glm::dot(north, cup));
    if(heading < 0)
        heading += T(2.0 * M_PI);

    float3 horz = ok
        ? glm::normalize(glm::cross(up, dir))
        : qs * detail::tvec3<T>(1, 0, 0);

    T roll = 0;
    if(ok) {
        const detail::tvec3<T> rup = glm::cross(horz, dir);

        roll = -atan2(glm::dot(horz, cup), -glm::dot(rup, cup));
    }

    return detail::tvec3<T>(heading, pitch, roll);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline uchar4 mul_int(const uchar4 a, const uchar4 b)
{
    return uchar4(
        ushort(a.x) * ushort(b.x) >> 8,
        ushort(a.y) * ushort(b.y) >> 8,
        ushort(a.z) * ushort(b.z) >> 8,
        ushort(a.w) * ushort(b.w) >> 8);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

} // end of namespace glm

template<typename T>
inline void operator +=(glm::detail::tquat<T> &dst, const glm::detail::tquat<T> &q)
{
    dst.x += q.x;
    dst.y += q.y;
    dst.z += q.z;
    dst.w += q.w;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename T>
inline void operator -=(glm::detail::tquat<T> &dst, const glm::detail::tquat<T> &q)
{
    dst.x -= q.x;
    dst.y -= q.y;
    dst.z -= q.z;
    dst.w -= q.w;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#endif // __OUTERRA_ENG_GLM_EXT_H__
