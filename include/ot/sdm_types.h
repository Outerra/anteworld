#pragma once
#ifndef __OUTERRA_SDM_TYPES_H__
#define __OUTERRA_SDM_TYPES_H__

#include "glm/glm_types.h"

namespace ot {
    struct sdm_vertex
    {
        float2 _vtx;    // 
        ushort2 _uv;    // normalized ushort2
        uint _alpha;   // 
    };
}

#endif // __OUTERRA_SDM_TYPES_H__