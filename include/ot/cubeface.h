
#ifndef __OT__CUBEFACE__HEADER_FILE__
#define __OT__CUBEFACE__HEADER_FILE__

#include <comm/commtypes.h>
#include <comm/mathf.h>
#include <comm/bitrange.h>
#include <comm/str.h>
#include <math.h>
#include "glm/glm_ext.h"

namespace ot {


inline double earth_sun_declination_deg(double day)
{
    return -23.45 * cos(2*M_PI/365*(day+10));
}


///Cube faces enumeration
enum ECubeFace {
    XMINUS, XPLUS, YMINUS, YPLUS, ZMINUS, ZPLUS
};

//  face crossings and orientation
//                                              2nd coord flip                                  sign change
//      +u  -u  +v  -v      +u  -u  +v  -v      +h  -h  +v  -v      -x  +x  -y  +y  -z  +z   -x +x -y +y -z +z
//     ---------------      --------------      --------------      ----------------------   -----------------
//  -x| +y  -y  -z  +z      -v  +v  -u  -u      -   -   -   +               -u  +u  +v  -v          1  1  1  0
//  +x| +y  -y  +z  -z      +v  -v  +u  +u      +   +   +   -               -u  +u  -v  +v          0  0  1  0
//  -y| +z  -z  -x  +x      -v  +v  -u  -u      -   -   -   +       +v  -v          -u  +u    1  0        1  1
//  +y| +z  -z  +x  -x      +v  -v  +u  +u      +   +   +   -       +v  -v          +u  -u    1  0        0  0
//  -z| +x  -x  -y  +y      -v  +v  -u  -u      -   -   -   +       -u  +u  +v  -v            1  1  1  0
//  +z| +x  -x  +y  -y      +v  -v  +u  +u      +   +   +   -       -u  +u  -v  +v            0  0  1  0

///cubeface displacement coefficient
extern double CUBEFACE_M;

///Quadrilateralized spherical cube OTC true mode (false for legacy mode)
extern bool QSC_OTC;


///Set QSC-OT coefficient to use
//@param qsc_ot false old approximate OT projection, true fixed OTC projection coefficient
const double& cubeface_qsc_otc( bool qsc_ot );


///Get approximate equatorial tile length for specific quad-tree level and planet radius
inline double cubeface_edge_length( double R, int level ) {
    return glm::ldexp_fast(M_PI_2*R, -level);
}

//@return floating point tile coordinates on face
inline float2 cubeface_level_coords(int h, int v, int level) {
    return float2(
        glm::ldexp_fast(float(h), level-31),
        glm::ldexp_fast(float(v), level-31));
}

///Find face where the 3D coords reside
//@param xyz 3D coordinates (don't have to be normalized)
//@return face id
template<class FLOAT>
inline int xyz_to_cubeface( const FLOAT xyz[3] )
{
    FLOAT yzxy[4] = { xyz[1], xyz[2], xyz[0], xyz[1] };
    FLOAT axyz[3] = { fabs(xyz[0]), fabs(xyz[1]), fabs(xyz[2]) };

    //find face
    int f;
    f = axyz[1] > axyz[0]  ?  1  :  0;
    f = axyz[2] > axyz[f]  ?  2  :  f;

    return (f<<1) + (xyz[f]>0 ? 1 : 0);
}


///Convert xyz 3D coordinates to cube-face coordinates of given face
//@param xyz 3D coordinates (do not have to be normalized)
//@param force_face face to use, -1 to deduce the face id
//@param face [out] 2D cube face coordinates in range <-1 .. 1>
//@return face id
template<class FLOAT>
inline int xyz_to_cubeface_face( const FLOAT xyz[3], int force_face, FLOAT face[2] )
{
    if(force_face < 0)
        force_face = xyz_to_cubeface(xyz);

    FLOAT yzxy[4] = { xyz[1], xyz[2], xyz[0], xyz[1] };

    int f = force_face >> 1;
    int plus = force_face & 1;

    //change sign
    yzxy[f+1] = plus ? yzxy[f+1] : -yzxy[f+1];

    const FLOAT M = FLOAT(CUBEFACE_M);

    //should be scaled so that
    // K*z = 1 + M * (1 - (K*x)^2) * (1 - (K*y)^2)
    //
    // K*z = 1 + M * (1 - (K*x)^2 - (K*y)^2 + K^4*x^2*y^2)
    // K^4*a + K^2*b + K*c + d = 0
    //
    // using Newton solver:
    // f'(K) = 4*a*K^3 + 2*b*K + c
    // K(n+1) = K(n) - f(K)/f'(K)

    FLOAT af = fabs(xyz[f]);

    FLOAT a = M * yzxy[f] * yzxy[f] * yzxy[f+1] * yzxy[f+1];
    FLOAT b = -M * (yzxy[f] * yzxy[f] + yzxy[f+1] * yzxy[f+1]);
    FLOAT c = -af;
    FLOAT d = 1 + M;

    FLOAT K = 1/af, fK, dK;
    FLOAT fKa, fKamax = FLT_MAX;
    do {
        FLOAT aKK = K*K*a;
        fK = K*(K*(b + aKK) + c) + d;
        fKa = fabs(fK);
        if(fKa > fKamax) {
            //this can happen with forced face conversion
            DASSERT( fabs(yzxy[f] * K) > 1 || fabs(yzxy[f+1] * K) > 1 );
            break;      //break if Newton can't converge here
        }
        fKamax = fKa;

        dK = K*(4*aKK + 2*b) + c;
        K = K - fK/dK;
    } while(fKa > 1.e-8);

    face[0] = yzxy[f] * K;
    face[1] = yzxy[f+1] * K;

    return (f<<1) + plus;
}


///Convert xyz 3D coordinates to cube-face coordinates
//@param xyz 3D coordinates (don't have to be normalized)
//@param face [out] 2D cube face coordinates in range <-1 .. 1>
//@return face id
template<class FLOAT>
inline int xyz_to_cubeface( const FLOAT xyz[3], FLOAT face[2] )
{
    return xyz_to_cubeface_face(xyz, -1, face);
}


///Convert xyz 3D coordinates to -0x40000000 .. 0x40000000 integer cube-face coordinates
//@param xyz 3D coordinates (don't have to be normalized)
//@param face [out] 2D cube face coordinates in range <-0x40000000 .. 0x40000000>
//@return face id
template<class FLOAT>
inline int xyz_to_cubeface_face( const FLOAT xyz[3], int force_face, int face[2] )
{
    FLOAT dface[2];
    int f = xyz_to_cubeface_face(xyz, force_face, dface);

    face[0] = int(floor(dface[0] * 0x40000000 + 0.5f));
    face[1] = int(floor(dface[1] * 0x40000000 + 0.5f));

    return f;
}


///Convert xyz 3D coordinates to -0x40000000 .. 0x40000000 integer cube-face coordinates
//@param xyz 3D coordinates (don't have to be normalized)
//@param face [out] 2D cube face coordinates in range <-0x40000000 .. 0x40000000>
//@return face id
template<class FLOAT>
inline int xyz_to_cubeface( const FLOAT xyz[3], int face[2] )
{
    FLOAT dface[2];
    int f = xyz_to_cubeface(xyz, dface);

    face[0] = int(floor(dface[0] * 0x40000000 + 0.5f));
    face[1] = int(floor(dface[1] * 0x40000000 + 0.5f));

    return f;
}

///Convert cube-face coordinates to 3D normalized coordinates
//@param f face id
//@param faceh horizontal face coordinate in range <-1 .. 1>
//@param facev vertical face coordinate in range <-1 .. 1>
//@param xyz [out] 3D normalized coordinates
//@return xyz
//@note face coordinates (u,v) are converted to 3D by normalization of
// (u, v, 1 + M * (1 - u^2) * (1 - v^2))
template<class FLOAT>
inline FLOAT* cubeface_to_xyz( int f, FLOAT faceh, FLOAT facev, FLOAT xyz[3] )
{
    FLOAT q0 = faceh*faceh;
    FLOAT q1 = facev*facev;

    const FLOAT M = FLOAT(CUBEFACE_M);

    FLOAT w = 1 + M * (1 - q0) * (1 - q1);
    FLOAT d = 1 / sqrt( q0 + q1 + w*w );

    FLOAT fv = f&1 ? facev : -facev;
    const FLOAT xyzxy[5] = { faceh, fv, f&1 ? w : -w, faceh, fv };

    const FLOAT* pxyz = xyzxy + 2 - (f>>1);
    xyz[0] = pxyz[0] * d;
    xyz[1] = pxyz[1] * d;
    xyz[2] = pxyz[2] * d;
    return xyz;
}

///Compute world space direction of u and v vector on face
template<class FLOAT>
inline void cubeface_uvdir( int f, FLOAT faceh, FLOAT facev, FLOAT udir[3], FLOAT vdir[3] )
{
    FLOAT q0 = faceh*faceh;
    FLOAT q1 = facev*facev;

    const FLOAT M = FLOAT(CUBEFACE_M);

    FLOAT w = 1 + M * (1 - q0) * (1 - q1);
    FLOAT d = 1 / sqrt( q0 + q1 + w*w );
    FLOAT dwv = 2*M*facev*(q0 - 1);
    FLOAT dwu = 2*M*faceh*(q1 - 1);
    FLOAT p = ::pow(w*w + q0 + q1, -1.5f);
    FLOAT ddv = -(dwv*w + facev) * p;
    FLOAT ddu = -(dwu*w + faceh) * p;

    FLOAT fv = FLOAT(f&1 ? 1 : -1);
    const FLOAT xyzxy[5] = { faceh, fv*facev, f&1 ? w : -w, faceh, fv*facev };
    const FLOAT dvxyzxy[5] = { 0, fv, f&1 ? dwv : -dwv, 0, fv };
    const FLOAT duxyzxy[5] = { 1, 0, f&1 ? dwu : -dwu, 1, 0 };

    const FLOAT* pxyz = xyzxy + 2 - (f>>1);
    const FLOAT* pdvxyz = dvxyzxy + 2 - (f>>1);
    vdir[0] = pxyz[0] * ddv + pdvxyz[0] * d;
    vdir[1] = pxyz[1] * ddv + pdvxyz[1] * d;
    vdir[2] = pxyz[2] * ddv + pdvxyz[2] * d;

    const FLOAT* pduxyz = duxyzxy + 2 - (f>>1);
    udir[0] = pxyz[0] * ddu + pduxyz[0] * d;
    udir[1] = pxyz[1] * ddu + pduxyz[1] * d;
    udir[2] = pxyz[2] * ddu + pduxyz[2] * d;
}

template<class FLOAT>
inline void cubeface_uvdir( int f, int horz, int vert, FLOAT udir[3], FLOAT vdir[3] )
{
    return cubeface_uvdir(f, FLOAT(horz)/0x40000000U, FLOAT(vert)/0x40000000U, udir, vdir);
}


///Convert cube-face coordinates to 3D normalized coordinates
//@param f face id
//@param face 2D face coordinates in range <-1 .. 1>
//@param xyz [out] 3D normalized coordinates
//@return xyz
//@note face coordinates (u,v) are converted to 3D by normalization of
// (u, v, 1 + M * (1 - u^2) * (1 - v^2))
template<class FLOAT>
inline FLOAT* cubeface_to_xyz( int f, const FLOAT face[2], FLOAT xyz[3] )
{
    return cubeface_to_xyz(f, face[0], face[1], xyz);
}

///Convert cube-face -0x40000000..0x40000000 coordinates to normalized 3D coordinates
template<class FLOAT>
inline FLOAT* cubeface_to_xyz( int f, int horz, int vert, FLOAT xyz[3] )
{
    //asserts disabled because generate_mesh needs slightly bigger range
    //DASSERT( horz >= -0x40000000  &&  horz <= 0x40000000 );
    //DASSERT( vert >= -0x40000000  &&  vert <= 0x40000000 );

    FLOAT fhv[2];
    fhv[0] = FLOAT(horz)/0x40000000U;
    fhv[1] = FLOAT(vert)/0x40000000U;

    return cubeface_to_xyz( f, fhv, xyz );
}



//@return the shortening coefficient - the smallest ratio between euclidian distance and unskewed line in u or v direction
//@note used to determine euclidian length that would give the same minimum du or dv
template<class FLOAT>
inline FLOAT cubeface_skew_coef( const FLOAT u, const FLOAT v )
{
    FLOAT uu = u*u;
    FLOAT vv = v*v;

    double3 p0, p1, p2;
    cubeface_to_xyz(0, u, v, &p0.x);
    cubeface_to_xyz(0, u+1e-6, v, &p1.x);
    cubeface_to_xyz(0, u, v+1e-6, &p2.x);

    double x = glm::dot(
        glm::normalize(p1-p0),
        glm::normalize(p2-p0));
    return glm::sqrtc(1 - x*x);
}

///Convert 3D coordinates to longitude and latitude angles
//@param xyz 3D coordinates
//@param lonlat [out] longitude (<-pi .. pi>) and latitude (<-pi/2 .. pi/2>) angles
//@return lonlat
template<class FLOAT>
inline FLOAT* xyz_to_lonlat_radians( const FLOAT xyz[3], FLOAT lonlat[2] )
{
    lonlat[0] = atan2( xyz[1], xyz[0] );
    lonlat[1] = atan2( xyz[2], sqrt(xyz[0]*xyz[0] + xyz[1]*xyz[1]) );

    return lonlat;
}

///Convert 3D coordinates to longitude and latitude angles
//@param xyz 3D coordinates
//@param lonlat [out] longitude (<-180 .. 180>) and latitude (<-90 .. 90>) angles
//@return lonlat
template<class FLOAT>
inline FLOAT* xyz_to_lonlat_degrees( const FLOAT xyz[3], FLOAT lonlatdeg[2] )
{
    FLOAT lonlat[2];
    xyz_to_lonlat_radians(xyz, lonlat);

    lonlatdeg[0] = lonlat[0] * 180.0/M_PI;
    lonlatdeg[1] = lonlat[1] * 180.0/M_PI;
    return lonlatdeg;
}

static const double rad_eq = 6378137.0;
static const double rad_pol = 6356752.3142;
static const double ba2 = rad_pol*rad_pol/(rad_eq*rad_eq);


///Convert longitude and latitude angles to 3D normalized coordinates
//@param lonlat longitude (<-pi .. pi>) and latitude (<-pi/2 .. pi/2>) angles
//@param xyz [out] 3D normalized coordinates
//@return xyz
template<class FLOAT>
inline FLOAT* lonlat_radians_to_xyz( FLOAT lon, FLOAT lat, FLOAT xyz[3], FLOAT radius=1 )
{
    //FLOAT latg = atan(ba2 * tan(lat));

    FLOAT fc = cos(lon);
    FLOAT fs = sin(lon);
    FLOAT ac = cos(lat);

    xyz[2] = radius * sin(lat);
    xyz[0] = radius * fc * ac;
    xyz[1] = radius * fs * ac;

    return xyz;
}

///Convert longitude and latitude angles to 3D normalized coordinates
//@param lonlat longitude (<-180 .. 180>) and latitude (<-90 .. 90>) angles
//@param xyz [out] 3D normalized coordinates
//@return xyz
template<class FLOAT>
inline FLOAT* lonlat_degrees_to_xyz( FLOAT londeg, FLOAT latdeg, FLOAT xyz[3], FLOAT radius=1 )
{
    FLOAT lon = londeg * FLOAT(M_PI/180.0);
    FLOAT lat = latdeg * FLOAT(M_PI/180.0);

    return lonlat_radians_to_xyz(lon, lat, xyz, radius);
}

///
struct ellipsoid
{
    ellipsoid()
        : a(6378137), f(1.0 / 298.257223563)
    {
        b = a * (1 - f);
        ecc2 = f * (2 - f);
        eps = ecc2 / (1 - ecc2);
        e1 = (1 - sqrt(1 - ecc2)) / (1 + sqrt(1 - ecc2));
    }

    double3 lonlat_radians_to_xyz( double lon, double lat ) const
    {
        double sla = sin(lat);
        double cla = cos(lat);

        double h = 0;    //height above ellipsoid
        double v = a / sqrt(1 - ecc2*sla*sla);

        double clo = cos(lon);
        double slo = sin(lon);

        return double3(
            (v + h) * clo * cla,
            (v + h) * slo * cla,
            ((1 - ecc2)*v + h) * sla);
    }

    double2 xyz_to_lonlat_radians( const double xyz[3] ) const
    {
        double p = sqrt(xyz[0] * xyz[0] + xyz[1] * xyz[1]);
        double q = atan2(xyz[2] * a, p * b);
        double sq = sin(q);
        double cq = cos(q);
        double lat = atan2(xyz[2] + eps * b * sq * sq * sq, p - ecc2 * a * cq * cq * cq);

        return double2(
            atan2(xyz[1], xyz[0]),
            lat);

        //to compute height above ellipsoid:
        //double sla = sin(lat);
        //double v = a / sqrt(1 – ecc2*sla*sla);
        //return p / cos(lat) - v;
    }

    double3 lonlat_degrees_to_xyz( double londeg, double latdeg ) const {
        double lon = londeg * (M_PI/180.0);
        double lat = latdeg * (M_PI/180.0);

        return lonlat_radians_to_xyz(lon, lat);
    }

    double2 xyz_to_lonlat_degrees( const double xyz[3] ) const {
        return glm::degrees(xyz_to_lonlat_radians(xyz));
    }

    double radius_lat_degrees(double lat) const {
        double sla = sin(glm::radians(lat));
        return a / sqrt(1 - ecc2*sla*sla);
    }


    double2 utm_to_lonlat_radians(double UTMNorthing, double UTMEasting, int UTMZone) const
    {
        static const double k0 = 0.9996;

        double x = UTMEasting - 500000.0;;
        double y = UTMNorthing;
        int8 zoneNumber = UTMZone < 0 ? -UTMZone : UTMZone;

        double longitudeOrigin = (zoneNumber - 1.0) * 6.0 - 180.0 + 3;

        if (UTMZone < 0)
            y -= 10000000.0;

        double m = y / k0;
        double mu = m / (a* (1.0 - ecc2 / 4.0 - 3.0 * ecc2 * ecc2 / 64.0 - 5.0 * glm::pow(ecc2, 3.0) / 256.0));
        double phi1Rad = mu + (3.0 * e1 / 2.0 - 27.0 * glm::pow(e1, 3.0) / 32.0) * glm::sin(2.0 * mu)
            + (21.0 * e1 * e1 / 16.0 - 55.0 * glm::pow(e1, 4.0) / 32.0)
            * glm::sin(4.0 * mu)
            + (151.0 * glm::pow(e1, 3.0) / 96.0) * glm::sin(6.0 * mu);
        double sphi1Rad = sin(phi1Rad);
        double sphi1Rad2 = sphi1Rad * sphi1Rad;
        double cphi1Rad = cos(phi1Rad);
        double tphi1Rad = sphi1Rad / cphi1Rad;

        double n = a / glm::sqrt(1.0 - ecc2 * sphi1Rad2);
        double t = tphi1Rad * tphi1Rad;
        double c = eps * glm::cos(phi1Rad) * cphi1Rad;
        double c2 = c * c;
        double r = a * (1.0 - ecc2) / glm::pow(1.0 - ecc2 * sphi1Rad2, 1.5);
        double d = x / (n * k0);
        double d2 = d * d;
        double d3 = d2 * d;
        double d5 = d3 * d2;

        double lat = phi1Rad - (n * tphi1Rad / r)
            * (d2/2 - (5 + 3*t + 10*c - 4*c2 - 9*eps) * d*d3/24
                + (61 + 90*t + 298*c + 45.0*t*t - 252*eps - 3*c2) * d*d5 / 720);

        double lon = longitudeOrigin * M_PI / 180
            + (d - (1 + 2*t + c) * d3/6 + (5 - 2*c + 28*t - 3*c2 + 8*eps + 24*t*t) * d5/120) / cphi1Rad;

        return double2(lon, lat);
    }

    double2 utm_to_lonlat_degrees(double UTMNorthing, double UTMEasting, int UTMZone) const
    {
        return utm_to_lonlat_radians(UTMNorthing, UTMEasting, UTMZone) * (180.0 / M_PI);
    }

    int lonlat_radians_to_utm(const double lon, const double lat,
        double& UTMNorthing, double& UTMEasting, int UTMZone = 0)
    {
        //converts lat/long to UTM coords.  Equations from USGS Bulletin 1532 
        static const double k0 = 0.9996;

        //Make sure the longitude is between -180.00 .. 179.9
        double londeg = glm::degrees(lon);
        londeg = (londeg + 180) - int((londeg + 180) / 360) * 360 - 180; // -180.00 .. 179.9;

        double lonrad = glm::radians(londeg);

        int zone = UTMZone != 0
            ? (UTMZone > 0 ? UTMZone : -UTMZone)    //forced zone
            : int((londeg + 180) / 6) + 1;
/*
        if (Lat >= 56.0 && Lat < 64.0 && lontmp >= 3.0 && lontmp < 12.0)
            zone = 32;

        // Special zones for Svalbard
        if (Lat >= 72.0 && Lat < 84.0)
        {
            if (lontmp >= 0.0  && lontmp < 9.0) zone = 31;
            else if (lontmp >= 9.0  && lontmp < 21.0) zone = 33;
            else if (lontmp >= 21.0 && lontmp < 33.0) zone = 35;
            else if (lontmp >= 33.0 && lontmp < 42.0) zone = 37;
        }*/

        double lon_origin = glm::radians(double((zone - 1) * 6 - 180 + 3));

        double sla = sin(lat);
        double cla = cos(lat);
        double N = a / sqrt(1 - ecc2*sla*sla);
        double T = sla*sla / (cla*cla);
        double C = eps*cla*cla;
        double A = cla * (lonrad - lon_origin);
        double A2 = A*A;

        double M = a*((1 - ecc2 / 4 - 3 * ecc2*ecc2 / 64 - 5 * ecc2*ecc2*ecc2 / 256)*lat
            - (3 * ecc2 / 8 + 3 * ecc2*ecc2 / 32 + 45 * ecc2*ecc2*ecc2 / 1024)*sin(2 * lat)
            + (15 * ecc2*ecc2 / 256 + 45 * ecc2*ecc2*ecc2 / 1024)*sin(4 * lat)
            - (35 * ecc2*ecc2*ecc2 / 3072)*sin(6 * lat));

        UTMEasting = (double)(k0*N*(A + (1 - T + C)*A2*A / 6
            + (5 - 18 * T + T*T + 72 * C - 58 * eps)*A2*A2*A / 120)
            + 500000.0);

        UTMNorthing = (double)(k0*(M + N*sla/cla*(A2 / 2 + (5 - T + 9 * C + 4 * C*C)*A2*A2 / 24
            + (61 - 58 * T + T*T + 600 * C - 330 * eps)*A2*A2*A2 / 720)));
        if (lat < 0)
            UTMNorthing += 10000000.0; //10000000 meter offset for southern hemisphere

        return lat < 0 ? -zone : zone;
    }

    int lonlat_degrees_to_utm(const double lon, const double lat,
        double& UTMNorthing, double& UTMEasting, int UTMZone = 0)
    {
        return lonlat_radians_to_utm(glm::radians(lon), glm::radians(lat), UTMNorthing, UTMEasting, UTMZone);
    }

    double a, b;
    double f;
    double ecc2;
    double eps;                         //< ecc prime squared, ecc2 / (1 - ecc2)
    double e1;
};


///Convert cube-face coordinates to longitude and latitude angles
//@param f face id
//@param h horizontal face coordinate in range <-1 .. 1>
//@param v vertical face coordinate in range <-1 .. 1>
//@param lonlat [out] longitude (<-pi .. pi>) and latitude (<-pi/2 .. pi/2>) angles
//@return lonlat
template<class FLOAT>
inline FLOAT* cubeface_to_lonlat_radians( int f, FLOAT h, FLOAT v, FLOAT lonlat[2] )
{
    FLOAT face[2] = {h, v};
    FLOAT xyz[3];
    cubeface_to_xyz(f, face, xyz);

    lonlat[0] = atan2( xyz[1], xyz[0] );
    lonlat[1] = atan2( xyz[2], sqrt(xyz[0]*xyz[0] + xyz[1]*xyz[1]) );

    return lonlat;
}

///Convert cube-face coordinates to longitude and latitude angles
//@param f face id
//@param face 2D face coordinates in range <-1 .. 1>
//@param lonlat [out] longitude (<-pi .. pi>) and latitude (<-pi/2 .. pi/2>) angles
//@return lonlat
template<class FLOAT>
inline FLOAT* cubeface_to_lonlat_radians( int f, const FLOAT face[2], FLOAT lonlat[2] )
{
    FLOAT xyz[3];
    cubeface_to_xyz(f, face, xyz);

    lonlat[0] = atan2( xyz[1], xyz[0] );
    lonlat[1] = atan2( xyz[2], sqrt(xyz[0]*xyz[0] + xyz[1]*xyz[1]) );

    return lonlat;
}

///Convert cube-face coordinates to longitude and latitude angles in degrees
//@param f face id
//@param h horizontal face coordinate in range <-1 .. 1>
//@param v vertical face coordinate in range <-1 .. 1>
//@param lonlat [out] longitude (<-pi .. pi>) and latitude (<-pi/2 .. pi/2>) angles
//@return lonlat
template<class FLOAT>
inline FLOAT* cubeface_to_lonlat_degrees( int f, FLOAT h, FLOAT v, FLOAT lonlat[2] )
{
    cubeface_to_lonlat_radians(f, h, v, lonlat);
    lonlat[0] *= FLOAT(180.0/M_PI);
    lonlat[1] *= FLOAT(180.0/M_PI);
    return lonlat;
}

///Convert cube-face coordinates to longitude and latitude angles in degrees
//@param f face id
//@param face 2D face coordinates in range <-1 .. 1>
//@param lonlat [out] longitude (<-pi .. pi>) and latitude (<-pi/2 .. pi/2>) angles
//@return lonlat
template<class FLOAT>
inline FLOAT* cubeface_to_lonlat_degrees( int f, const FLOAT face[2], FLOAT lonlat[2] )
{
    cubeface_to_lonlat_radians(f, face, lonlat);
    lonlat[0] *= FLOAT(180.0/M_PI);
    lonlat[1] *= FLOAT(180.0/M_PI);
    return lonlat;
}

///Convert cube-face coordinates to longitude and latitude angles in degrees
//@param f face id
//@param horz,vert face coordinates in -0x40000000..0x40000000 range
template<class FLOAT>
inline FLOAT* cubeface_to_lonlat_degrees( int f, int horz, int vert, FLOAT lonlat[2] )
{
    FLOAT fhv[2];
    fhv[0] = FLOAT(horz)/0x40000000U;
    fhv[1] = FLOAT(vert)/0x40000000U;

    return cubeface_to_lonlat_degrees(f, fhv, lonlat);
}


///Convert cube-face coordinates to stereoscopic polar coordinates
//@param f face id
//@param h horizontal face coordinate in range <-1 .. 1>
//@param v vertical face coordinate in range <-1 .. 1>
//@param sp [out] x and y polar coords
//@return sp
template<class FLOAT>
inline FLOAT* cubeface_to_polar( int f, FLOAT h, FLOAT v, FLOAT sp[2] )
{
    FLOAT xyz[3];
    cubeface_to_xyz(f, h, v, xyz);

    FLOAT d = FLOAT(1.0) / (FLOAT(1.0) + sqrt(FLOAT(1) - xyz[0]*xyz[0] - xyz[1]*xyz[1]));
    sp[0] = xyz[0] * d;
    sp[1] = xyz[1] * d;
    return sp;
}


///Convert longitude and latitude angles to cube-face coordinates
//@param lonlat longitude (<-pi .. pi>) and latitude (<-pi/2 .. pi/2>) angles
//@param face [out] 2D cube face coordinates in range <-1 .. 1>
//@return face id
template<class FLOAT>
inline int lonlat_radians_to_cubeface( FLOAT lon, FLOAT lat, FLOAT face[2] )
{
    FLOAT xyz[3];

    lonlat_radians_to_xyz(lon, lat, xyz);
    return xyz_to_cubeface(xyz, face);
}

///Convert longitude and latitude angles to i1.30 integer cube-face coordinates
//@param lonlat longitude (<-pi .. pi>) and latitude (<-pi/2 .. pi/2>) angles
//@param face [out] 2D cube face coordinates in range <-0x40000000 .. 0x40000000>
//@return face id
template<class FLOAT>
inline int lonlat_radians_to_cubeface( FLOAT lon, FLOAT lat, int face[2] )
{
    FLOAT xyz[3];

    lonlat_radians_to_xyz(lon, lat, xyz);
    return xyz_to_cubeface(xyz, face);
}

///Convert longitude and latitude angles to i1.30 integer cube-face coordinates
//@param lonlat longitude (<-180 .. 180>) and latitude (<-90 .. 90>) angles
//@param face [out] 2D cube face coordinates in range <-0x40000000 .. 0x40000000>
//@return face id
template<class FLOAT>
inline int lonlat_degrees_to_cubeface( FLOAT lon, FLOAT lat, int face[2] )
{
    FLOAT xyz[3];

    lonlat_degrees_to_xyz(lon, lat, xyz);
    return xyz_to_cubeface(xyz, face);
}


///Get the u-direction on the cube face
inline const float* xcubeface_udir( int face )
{
    // -x:  0  1  0
    // +x:  0  1  0
    // -y:  0  0  1
    // +y:  0  0  1
    // -z:  1  0  0
    // +z:  1  0  0
    static const float udir[5] = { 1, 0, 0, 1, 0 };

    return udir + 2 - (face>>1);
}

///Get the v-direction on the cube face
inline const float* xcubeface_vdir( int face )
{
    // -x:  0  0 -1
    // +x:  0  0  1
    // -y: -1  0  0
    // +y:  1  0  0
    // -z:  0 -1  0
    // +z:  0  1  0
    static const float vdirm[5] = { 0, -1, 0, 0, -1 };
    static const float vdirp[5] = { 0, 1, 0, 0, 1 };

    return (face&1 ? vdirp : vdirm) + 2 - (face>>1);
}

///
inline const char* cubeface_name( int face, bool newface = false ) {
    static const char* names1[] = { "-x", "+x", "-y", "+y", "-z", "+z" };
    static const char* names2[] = { "xn", "xp", "yn", "yp", "zn", "zp" };
    return newface ? names2[face] : names1[face];
}

///Convert cube-face i1.30 coordinates to cache path
inline void cubeface_append_cache_path( int face, int h, int v, int tolevel, coid::charstr& path )
{
    path += cubeface_name(face);
    path.append('/');

    uint fh = h + 0x40000000;
    uint fv = v + 0x40000000;
    uint msk = int(0x80000000) >> (QSC_OTC ? tolevel+1 : tolevel);
    fh &= msk;
    fv &= msk;

    static const char* hex = "0123456789ABCDEF";
    const int N = QSC_OTC ? 31 : 32;

    for( int i=0; i<=tolevel; i+=4 )
    {
        path.append(hex[(fh>>(N-4-i))&15]);
        path.append(hex[(fv>>(N-4-i))&15]);
        path.append('/');
    }
}


///Return neighbouring face id and neighbouring edge
//@param face original face id
//@param h can be -,+ to identify an edge in horizontal direction, 0 if vertical direction is queried
//@param v can be -,+ to identify an edge in vertical direction, 0 if horizontal direction is queried
inline int cubeface_neighbour_rect( int face, double& h, double& dh, double& v, double& dv )
{
    //      +h  -h  +v  -v      +h  -h  +v  -v      +h  -h  +v  -v
    //     ---------------      --------------      --------------
    //  -x| +y  -y  -z  +z      -v  +v  -h  -h      -   -   -   +
    //  +x| +y  -y  +z  -z      +v  -v  +h  +h      +   +   +   -
    //  -y| +z  -z  -x  +x      -v  +v  -h  -h      -   -   -   +
    //  +y| +z  -z  +x  -x      +v  -v  +h  +h      +   +   +   -
    //  -z| +x  -x  -y  +y      -v  +v  -h  -h      -   -   -   +
    //  +z| +x  -x  +y  -y      +v  -v  +h  +h      +   +   +   -
    //

    int ngf;
    double hb = h, he = h+dh, vb = v, ve = v+dv;
    double nhb, nhe, nvb, nve;

    if(he>1) {
        ngf = ((face|1) + 2) % 6;
        nvb = -(2 - hb);
        nve = -(2 - he);
        nhb = -vb;
        nhe = -ve;
    }
    else if(hb<-1) {
        ngf = ((face&6) + 2) % 6;
        nvb = +(2 + hb);
        nve = +(2 + he);
        nhb = -vb;
        nhe = -ve;
    }
    else if(ve>1) {
        ngf = (face + 4) % 6;
        nhb = -(2 - vb);
        nhe = -(2 - ve);
        nvb = -hb;
        nve = -he;
    }
    else if(vb<-1) {
        ngf = ((face^1) + 4) % 6;
        nhb = -(2 + vb);
        nhe = -(2 + ve);
        nvb = hb;
        nve = he;
    }
    else
        return face;

    hb = (face&1) != 0 ? -nhb : nhb;//face_change_sign(nhb, face&1);
    he = (face&1) != 0 ? -nhe : nhe;//face_change_sign(nhe, face&1);
    vb = (face&1) != 0 ? -nvb : nvb;//face_change_sign(nvb, face&1);
    ve = (face&1) != 0 ? -nve : nve;//face_change_sign(nve, face&1);

    h = hb;
    dh = he - hb;
    v = vb;
    dv = ve - vb;

    return ngf;
}

////////////////////////////////////////////////////////////////////////////////
///Compact 64-bit spherical tile/position address
//@note max.resolution ~1cm on sphere with Earth's radius
struct spherecoord
{
    union {
        uint64 _bits;
        struct {
            uint64 _fchorz : 31;        //< horizontal coordinate, level indicated by the lowest bit set
            uint64 _fcvert : 30;        //< vertical coordinate on the face
            uint64 _face : 3;           //< face id, ECubeFace
        };
    };

    spherecoord()
        : _bits(UMAX64)
    {}

    explicit spherecoord( uint64 id )
        : _bits(id)
    {}

    ///Construct spherecoord from face, level and -0x40000000..0x40000000 face coordinates
    spherecoord( uint face, int fchorz, int fcvert, uint level ) {
        set_face_coords(face, fchorz, fcvert, level);
    }

    ///Construct spherecoord from face, level and 0..0x80000000 face coordinates
    spherecoord( uint face, uint fchorz, uint fcvert, uint level ) {
        set_face_coords(face, fchorz, fcvert, level);
    }

    ///Construct from ECEF point
    //@param xyz position
    //@param force_face optional face to constraint to
    explicit spherecoord( const double3& xyz, int force_face=-1 ) {
        int hv[2];
        int face = xyz_to_cubeface_face(&xyz.x, force_face, hv);

        set_face_coords(face, hv[0], hv[1], 30);
    }

    ///Set face coords (in -x40000000..0x40000000 range)
    //@param level max level 30
    void set_face_coords( uint face, int fchorz, int fcvert, uint level )
    {
        DASSERT( fchorz >= -0x40000000 && fchorz <= 0x40000000
            &&   fcvert >= -0x40000000 && fcvert <= 0x40000000 );

        set_face_coords(face, uint(fchorz + 0x40000000), uint(fcvert + 0x40000000), level);
    }

    ///Set face coords (in 0..0x80000000 range)
    //@param level max level 30
    void set_face_coords( uint face, uint fchorz, uint fcvert, uint level )
    {
        //DASSERT( fchorz <= 0x80000000U && fcvert <= 0x80000000U );
        DASSERT( level <= 30 );
        _face = face;

        //avoid overflow on the edge
        fchorz -= fchorz >> 31;
        fcvert -= fcvert >> 31;

        //cut off stuff below the level
        fchorz &= int(0x80000000) >> level;
        fcvert &= int(0x80000000) >> level;

        _fchorz =  fchorz + (0x40000000U>>level);
        _fcvert = (fcvert + (0x40000000U>>level)) >> 1;
    }

    operator uint64() const { return _bits; }

    bool invalid() const { return _face>=6; }

    //@return manhattan distance in face coord units
    uint64 manhattan_distance( const spherecoord& sc ) const {
        if(sc._face == _face)
            return abs(int(fchorz() - sc.fchorz())) + abs(int(fcvert() - sc.fcvert()));
        
        if((_face^1) == sc._face) {
            DASSERT(0);
            return UMAX64;  //TODO
        }

        //     -x  +x  -y  +y  -z  +z
        //    -----------------------
        // -x|         -u  +u  +v  -v
        // +x|         -u  +u  -v  +v
        // -y| +v  -v          -u  +u
        // +y| +v  -v          +u  -u
        // -z| -u  +u  +v  -v
        // +z| -u  +u  -v  +v

        static const int8 U[6*6] = {
            0, 0,-1, 1, 0, 0,
            0, 0,-1, 1, 0, 0,
            0, 0, 0, 0,-1, 1,
            0, 0, 0, 0, 1,-1,
           -1, 1, 0, 0, 0, 0,
           -1, 1, 0, 0, 0, 0
        };
        static const int8 V[6*6] = {
            0, 0, 0, 0, 1,-1,
            0, 0, 0, 0,-1, 1,
            1,-1, 0, 0, 0, 0,
            1,-1, 0, 0, 0, 0,
            0, 0, 1,-1, 0, 0,
            0, 0,-1, 1, 0, 0
        };

        uint k = face()*6 + sc.face();
        return
            + abs(-U[k]*0x80000000LL + int64(fchorz()) - int64(sc.fcvert()))
            + abs(-V[k]*0x80000000LL + int64(fcvert()) - int64(sc.fchorz()));
    }

    //@return tile level, provided the spherecoord stores tile midpoint coordinates
    int level() const {
        return _bits
            ? 30 - lsb_bit_set(_fchorz)
            : -1;
    }

    //@{ return unsigned face coordinates (0 .. 0x80000000)
    uint fchorz() const { return uint(_fchorz); }
    uint fcvert() const { return uint(_fcvert<<1); }
    //@}

    //@{ return centered face coordinates (-0x40000000 to 0x40000000)
    int horz() const { return uint(_fchorz) - 0x40000000; }
    int vert() const { return uint(_fcvert<<1) - 0x40000000; }
    //@}


    int* coords( int uv[2] ) const {
        uv[0] = horz();
        uv[1] = vert();
        return uv;
    }

    //@param force_face face to reference the coordinates to
    int* coords_on_face( int force_face, int uv[2] ) const {
        if(force_face == _face)
            return coords(uv);

        double xyz[3];
        cubeface_to_xyz(_face, horz(), vert(), xyz);
        xyz_to_cubeface_face(xyz, force_face, uv);
        return uv;
    }

    ///Compute 0..1 tile space coordinates of point for given parent tile level
    //@return false if the point level is lower than the required
    bool tile_coords( int lev, float uv[2] ) const {
        int levc = level();
        uint h = fchorz();
        uint v = fcvert();
        h <<= 1 + lev;
        v <<= 1 + lev;

        uv[0] = (float)glm::ldexp_fast(double(h), -32);
        uv[1] = (float)glm::ldexp_fast(double(v), -32);

        return levc >= lev;
    }

    ECubeFace face() const { DASSERT(_face<6); return (ECubeFace)_face; }

    //@return ECEF point on unit sphere
    double3 xyz() const {
        double3 val;
        cubeface_to_xyz(_face, horz(), vert(), &val.x);
        return val;
    }

    //@return lon/lat coordinates
    double2 lonlat_degrees() const {
        double2 val;
        cubeface_to_lonlat_degrees(_face, horz(), vert(), &val.x);
        return val;
    }

    //Convert real size on given radius to face coordinate delta
    float face_coord_delta( double R, float size ) const {
        //2*pi*R ~= 4*0x80000000
        //pi*R ~= 1<<32
        return float(glm::ldexp_fast(size / (M_PI*R), 32));
    }
};



} //namespace ot

#endif //__OT__CUBEFACE__HEADER_FILE__
