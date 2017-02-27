#pragma once

#include "glm_types.h"
#include <LinearMath/btScalar.h>
#include <LinearMath/btVector3.h>
#include <LinearMath/btMatrix3x3.h>

namespace bt {

inline ::float3 tofloat3( const ::btVector3& v ) {
    return ::float3(reinterpret_cast<const ::double3&>(v));
}

inline const ::double3& todouble3( const ::btVector3& v ) {
    return *reinterpret_cast<const ::double3*>(&v);
}

inline ::quat toquat( const ::btQuaternion& bq ) {
    return quat(float(bq.w()), float(bq.x()), float(bq.y()), float(bq.z()));
}


inline const ::btVector3& toVector3( const ::double3& v ) {
    return *reinterpret_cast<const ::btVector3*>(&v);
}

inline ::btVector3 toVector3( const ::float3& v ) {
    return ::btVector3(v.x, v.y, v.z);
}


inline float dot( const ::btVector3& v1, const ::float3& v2 ) {
    return float(v1[0]*v2.x + v1[1]*v2.y + v1[2]*v2.z);
}

inline double dotd( const ::btVector3& v1, const ::float3& v2 ) {
    return v1[0]*v2.x + v1[1]*v2.y + v1[2]*v2.z;
}


inline ::float3 mul(const ::btMatrix3x3& m, const ::float3& v) {
    return ::float3(dot(m[0], v), dot(m[1], v), dot(m[2], v));
}

inline ::double3 muld(const ::btMatrix3x3& m, const ::float3& v) {
    return ::double3(dotd(m[0], v), dotd(m[1], v), dotd(m[2], v));
}

} //namespace bt
