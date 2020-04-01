
#ifndef __OT_GLM_META_H__
#define __OT_GLM_META_H__

#include "glm_types.h"
#include <comm/binstream/binstream.h>
#include <comm/metastream/metastream.h>

////////////////////////////////////////////////////////////////////////////////

COID_METABIN_OP2D(float2,x,y,0.0f,0.0f)
COID_METABIN_OP2D(double2,x,y,0.0,0.0)
COID_METABIN_OP2D(int2,x,y,0,0)
COID_METABIN_OP2D(i16vec2,x,y,0,0)
COID_METABIN_OP2D(i8vec2,x,y,0,0)
COID_METABIN_OP2D(uint2,x,y,0,0)
COID_METABIN_OP2D(u16vec2,x,y,0,0)
COID_METABIN_OP2D(u8vec2,x,y,0,0)
COID_METABIN_OP2D(bool2,x,y,false,false)

COID_METABIN_OP3D(float3,x,y,z,0.0f,0.0f,0.0f)
COID_METABIN_OP3D(double3,x,y,z,0.0,0.0,0.0)
COID_METABIN_OP3D(int3,x,y,z,0,0,0)
COID_METABIN_OP3D(i16vec3,x,y,z,0,0,0)
COID_METABIN_OP3D(i8vec3,x,y,z,0,0,0)
COID_METABIN_OP3D(uint3,x,y,z,0,0,0)
COID_METABIN_OP3D(u16vec3,x,y,z,0,0,0)
COID_METABIN_OP3D(u8vec3,x,y,z,0,0,0)
COID_METABIN_OP3D(bool3,x,y,z,false,false,false)

COID_METABIN_OP4D(float4,x,y,z,w,0.0f,0.0f,0.0f,0.0f)
COID_METABIN_OP4D(double4,x,y,z,w,0.0,0.0,0.0,0.0)
COID_METABIN_OP4D(int4,x,y,z,w,0,0,0,0)
COID_METABIN_OP4D(i16vec4,x,y,z,w,0,0,0,0)
COID_METABIN_OP4D(i8vec4,x,y,z,w,0,0,0,0)
COID_METABIN_OP4D(uint4,x,y,z,w,0,0,0,0)
COID_METABIN_OP4D(u16vec4,x,y,z,w,0,0,0,0)
COID_METABIN_OP4D(u8vec4,x,y,z,w,0,0,0,0)
COID_METABIN_OP4D(bool4,x,y,z,w,false,false,false,false)

COID_METABIN_OP4(quat,x,y,z,w)
COID_METABIN_OP4(dquat,x,y,z,w)


COID_METABIN_OP2A(float2x2,float2)
COID_METABIN_OP2A(float3x2,float3)
COID_METABIN_OP2A(float4x2,float4)

COID_METABIN_OP3A(float2x3,float2)
COID_METABIN_OP3A(float3x3,float3)
COID_METABIN_OP3A(float4x3,float4)

COID_METABIN_OP4A(float2x4,float2)
COID_METABIN_OP4A(float3x4,float3)
COID_METABIN_OP4A(float4x4,float4)


#endif //__OT_GLM_META_H__


//include glm v8 streamer helpers
#ifdef __COID_COMM_FMTSTREAM_V8__HEADER_FILE__
#include "glm_meta_v8.h"
#endif
