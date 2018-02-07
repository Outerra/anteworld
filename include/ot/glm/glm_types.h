
#ifndef _OT_GLM_TYPES_H_
#define _OT_GLM_TYPES_H_

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <comm/commtypes.h>

//vectors

typedef glm::vec2 float2;
typedef glm::vec3 float3;
typedef glm::vec4 float4;

typedef glm::dvec2 double2;
typedef glm::dvec3 double3;
typedef glm::dvec4 double4;

typedef glm::ivec2 int2;
typedef glm::ivec3 int3;
typedef glm::ivec4 int4;

typedef glm::uvec2 uint2;
typedef glm::uvec3 uint3;
typedef glm::uvec4 uint4;

typedef glm::bvec2 bool2;
typedef glm::bvec3 bool3;
typedef glm::bvec4 bool4;

typedef glm::detail::tvec4<int64> i64vec4;
typedef glm::detail::tvec3<int64> i64vec3;
typedef glm::detail::tvec2<int64> i64vec2;

typedef glm::detail::tvec4<uint64> u64vec4;
typedef glm::detail::tvec3<uint64> u64vec3;
typedef glm::detail::tvec2<uint64> u64vec2;

typedef glm::detail::tvec4<int16> short4;
typedef glm::detail::tvec3<int16> short3;
typedef glm::detail::tvec2<int16> short2;

typedef glm::detail::tvec4<uint16> ushort4;
typedef glm::detail::tvec3<uint16> ushort3;
typedef glm::detail::tvec2<uint16> ushort2;

typedef glm::detail::tvec4<int8> schar4;
typedef glm::detail::tvec3<int8> schar3;
typedef glm::detail::tvec2<int8> schar2;

typedef glm::detail::tvec4<uint8> uchar4;
typedef glm::detail::tvec3<uint8> uchar3;
typedef glm::detail::tvec2<uint8> uchar2;

typedef glm::hvec4 half4;
typedef glm::hvec3 half3;
typedef glm::hvec2 half2;
typedef glm::detail::half half;

//matrices

typedef glm::mat2x2 float2x2;
typedef glm::mat2x3 float3x2;
typedef glm::mat2x4 float4x2;

typedef glm::mat3x2 float2x3;
typedef glm::mat3x3 float3x3;
typedef glm::mat3x4 float4x3;

typedef glm::mat4x2 float2x4;
typedef glm::mat4x3 float3x4;
typedef glm::mat4x4 float4x4;


typedef glm::dmat2x2 double2x2;
typedef glm::dmat2x3 double3x2;
typedef glm::dmat2x4 double4x2;

typedef glm::dmat3x2 double2x3;
typedef glm::dmat3x3 double3x3;
typedef glm::dmat3x4 double4x3;

typedef glm::dmat4x2 double2x4;
typedef glm::dmat4x3 double3x4;
typedef glm::dmat4x4 double4x4;

// quaternions

using glm::quat;
using glm::dquat;

#endif //_OT_GLM_TYPES_H_
