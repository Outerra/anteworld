#pragma once

#include <glm/glm.hpp>
#include <comm/commtypes.h>


//////////////////////////////////////////////
//				  PREFIXES			        //
//////////////////////////////////////////////
//											//
// closest_point - result is clostest		//
//			on obj2							//
//											//
// intersects - bool result, only check if  //
//			two objects intersecty			//
//											//
// collide - result is contact point		//
//		and contact normal					//
//											//
//////////////////////////////////////////////
// res_type prefix_obj1_obj2(type1 arg1...) //
//////////////////////////////////////////////

namespace coal
{
	enum EConvexEdge
	{
		ceEdge01Convex = 1,
		ceEdge12Convex = 1 << 1,
		ceEdge20Convex = 1 << 2
	};

    enum EVoronoiFeature {
        vfVertex0,
        vfVertex1,
        vfVertex2,
        vfEdge01,
        vfEdge12,
        vfEdge20,
        vfInside,
        vfCount
    };

    static const float small_angle_cos = 1.0f;
    static const float FLOAT_EPS = 0.000001f;

    //////////////////////////////////////////////////////////////

    inline EVoronoiFeature uvw_to_voronoi_feature(float u, float v, float w) {
        const float FLOAT_EPS = 0.0000001f;
        if (u < FLOAT_EPS) {
            
            if (v < FLOAT_EPS) {
                return vfVertex2;
            }
            else if(w < FLOAT_EPS) {
                return vfVertex1;
            }
            else {
                return vfEdge12;
            }
        }
        else {
            if (v < FLOAT_EPS) {
                if (w < FLOAT_EPS) {
                    return vfVertex0;
                }
                else {
                    return vfEdge20;
                }
            }

            if (w < FLOAT_EPS) {
                return vfEdge01;
            }
        }

        return vfInside;
    }

    //////////////////////////////////////////////////////////////

    inline bool is_valid_barycentric_coord(float u, float v)
    {
        return u >= -0.0001f && u <= 1.0001f && v >= -0.0001f && v <= 1.0001f && (u + v) < 1.0002f;
    }

    //////////////////////////////////////////////////////////////
    
    /*
    Christer Ericson’s Real-time Collision Detection
    Page 46, 3.4 Barycentric Coordinates
    */
    
    inline void calculate_barycentric_coords(const float3& p,
        const float3& a,
        const float3& b,
        const float3& c,
        float& u,
        float& v,
        float& w) 
    {
        const float3 ab = b - a;
        const float3 ac = c - a;
        const float3 ap = p - a;
        const float d00 = glm::dot(ab, ab);
        const float d01 = glm::dot(ab, ac);
        const float d11 = glm::dot(ac, ac);
        const float d20 = glm::dot(ap, ab);
        const float d21 = glm::dot(ap, ac);
        const float denom_inv = 1.f / (d00 * d11 - d01 * d01);
        v = (d11 * d20 - d01 * d21) * denom_inv;
        w = (d00 * d21 - d01 * d20) * denom_inv;
        u = 1.f - u - v;
    }

    inline void calculate_barycentric_coords(const float3& ab,
        const float3& ac,
        const float3& ap,
        float& u,
        float& v,
        float& w)
    {
        const float d00 = glm::dot(ab, ab);
        const float d01 = glm::dot(ab, ac);
        const float d11 = glm::dot(ac, ac);
        const float d20 = glm::dot(ap, ab);
        const float d21 = glm::dot(ap, ac);
        const float denom_inv = 1.f / (d00 * d11 - d01 * d01);
        v = (d11 * d20 - d01 * d21) * denom_inv;
        w = (d00 * d21 - d01 * d20) * denom_inv;
        u = 1.f - u - v;
    }

    //////////////////////////////////////////////////////////////
    
    inline bool is_contact_on_convex_edge(EVoronoiFeature vf, uint8 triangle_flags) {
        switch (vf) {
        case coal::vfVertex0:
            if (triangle_flags & (coal::ceEdge01Convex | coal::ceEdge20Convex)) {
                return true;
            }
            break;
        case coal::vfVertex1:
            if (triangle_flags & (coal::ceEdge01Convex | coal::ceEdge12Convex)) {
                return true;
            }
            break;
        case coal::vfVertex2:
            if (triangle_flags & (coal::ceEdge20Convex | coal::ceEdge12Convex)) {
                return true;
            }
            break;
        case coal::vfEdge01:
            if (triangle_flags & coal::ceEdge01Convex) {
                return true;
            }
            break;
        case coal::vfEdge12:
            if (triangle_flags & coal::ceEdge12Convex) {
                return true;
            }
            break;
        case coal::vfEdge20:
            if (triangle_flags & coal::ceEdge20Convex) {
                return true;
            }
            break;
        }

        return false;
    }

	//////////////////////////////////////////////////////////////

	inline glm::vec3 closest_point_point_triangle(const glm::vec3& point_p,
		const glm::vec3& triangle_a, 
		const glm::vec3& triangle_b, 
		const glm::vec3& triangle_c)
	{
		glm::vec3 ab = triangle_b - triangle_a;
		glm::vec3 ac = triangle_c - triangle_a;
		glm::vec3 bc = triangle_c - triangle_b;
		
		float snom = glm::dot(point_p - triangle_a, ab), sdenom = glm::dot(point_p - triangle_b, triangle_a - triangle_b);
		
		float tnom = glm::dot(point_p - triangle_a, ac), tdenom = glm::dot(point_p - triangle_c, triangle_a - triangle_c);
		if (snom <= 0.0f && tnom <= 0.0f) {
			return triangle_a;
		}
		 
		 
		float unom = glm::dot(point_p - triangle_b, bc), udenom = glm::dot(point_p - triangle_c, triangle_b - triangle_c);
		if (sdenom <= 0.0f && unom <= 0.0f) {
			return triangle_b;
		}
		if (tdenom <= 0.0f && udenom <= 0.0f) {
			return triangle_c;
		}
		
		glm::vec3 n = glm::cross(triangle_b - triangle_a, triangle_c - triangle_a);

		float vc = glm::dot(n, glm::cross(triangle_a - point_p, triangle_b - point_p));
		
		if (vc <= 0.0f && snom >= 0.0f && sdenom >= 0.0f) {
			return triangle_a + snom / (snom + sdenom) * ab;
		}

		float va = glm::dot(n, glm::cross(triangle_b - point_p, triangle_c - point_p));
		
		if (va <= 0.0f && unom >= 0.0f && udenom >= 0.0f) {
			return triangle_b + unom / (unom + udenom) * bc;
		}

		float vb = glm::dot(n, glm::cross(triangle_c - point_p, triangle_a - point_p));

		if (vb <= 0.0f && tnom >= 0.0f && tdenom >= 0.0f) {
			return triangle_a + tnom / (tnom + tdenom) * ac;
		}

		float u = va / (va + vb + vc);
		float v = vb / (va + vb + vc);
		float w = 1.0f - u - v;

		return u * triangle_a + v * triangle_b + w * triangle_c;
	}

	inline void closest_point_point_triangle(const glm::vec3 & point_p,
	                                  const glm::vec3 & triangle_a,
	                                  const glm::vec3 & triangle_b,
	                                  const glm::vec3 & triangle_c,
	                                  const glm::vec3 & triangle_an,
	                                  const glm::vec3 & triangle_bn,
	                                  const glm::vec3 & triangle_cn,
	                                  glm::vec3 & result_point,
	                                  glm::vec3 & result_normal)
	{
		glm::vec3 ab = triangle_b - triangle_a;
		glm::vec3 ac = triangle_c - triangle_a;
		glm::vec3 bc = triangle_c - triangle_b;
	 
		float snom = glm::dot(point_p - triangle_a, ab), sdenom = glm::dot(point_p - triangle_b, triangle_a - triangle_b);
		
		float tnom = glm::dot(point_p - triangle_a, ac), tdenom = glm::dot(point_p - triangle_c, triangle_a - triangle_c);
		if (snom <= 0.0f && tnom <= 0.0f) {
			result_point = triangle_a;
			result_normal = triangle_an;
			return;
		}

		float unom = glm::dot(point_p - triangle_b, bc), udenom = glm::dot(point_p - triangle_c, triangle_b - triangle_c);
		if (sdenom <= 0.0f && unom <= 0.0f) {
			result_point = triangle_b;
			result_normal = triangle_bn;
			return;
		}
		if (tdenom <= 0.0f && udenom <= 0.0f) {
			result_point = triangle_c;
			result_normal = triangle_cn; 
			return;
		}
		
		glm::vec3 n = glm::cross(triangle_b - triangle_a, triangle_c - triangle_a);

		float vc = glm::dot(n, glm::cross(triangle_a - point_p, triangle_b - point_p));
		
		if (vc <= 0.0f && snom >= 0.0f && sdenom >= 0.0f) {
			result_point = triangle_a + snom / (snom + sdenom) * ab;
			result_normal = glm::normalize(triangle_bn * (snom / (snom + sdenom)) + triangle_an * (sdenom / (snom + sdenom)));
			return;
		}

		float va = glm::dot(n, glm::cross(triangle_b - point_p, triangle_c - point_p));
		
		if (va <= 0.0f && unom >= 0.0f && udenom >= 0.0f) {
			result_point = triangle_b + unom / (unom + udenom) * bc;
			result_normal = glm::normalize(triangle_cn * (unom / (unom + udenom)) + triangle_bn * (udenom / (unom + udenom)));
			return;
		}


		float vb = glm::dot(n, glm::cross(triangle_c - point_p, triangle_a - point_p));

		if (vb <= 0.0f && tnom >= 0.0f && tdenom >= 0.0f) {
			result_point = triangle_a + tnom / (tnom + tdenom) * ac;
			result_normal = glm::normalize(triangle_cn * (tnom / (tnom + tdenom)) + triangle_an * (tdenom / (tnom + tdenom)));
			return;
		}


		float u = va / (va + vb + vc);
		float v = vb / (va + vb + vc);
		float w = 1.0f - u - v;

		result_point = u * triangle_a + v * triangle_b + w * triangle_c;
		result_normal = glm::normalize(u * triangle_an + v * triangle_bn + w * triangle_cn);
	}

    template <class T> 
    inline bool intersects_sphere_aabb(const glm::detail::tvec3<T> sphere_center,
		T sphere_radius,
		const glm::detail::tvec3<T>& aabb_origin,
		const glm::detail::tvec3<T>& aabb_half,
		T * dist)
	{
        glm::detail::tvec3<T> rel = sphere_center - aabb_origin;
		T dmin = 0;

		T r2 = sphere_radius*sphere_radius;

		for (int i = 0; i < 3; i++) {
			if (rel[i] < -aabb_half[i]) {
				dmin += glm::pow(rel[i] + aabb_half[i], (T)2.0);
			}
			else if (rel[i] > aabb_half[i]) {
				dmin += glm::pow(rel[i] - aabb_half[i], (T)2.0f);
			}
		}

		if (dmin <= r2) {
			if (dist) {
				*dist = glm::sqrt(dmin);
			}
			return true;
		}
		else {
			return false;
		}
	}

	inline bool intersects_aabb_plane(const glm::vec3 & aabb_origin,
		const glm::vec3 & aabb_half,
		float plane_d,
		const glm::vec3 & plane_normal)
	{
		float r = aabb_half[0] * glm::abs(plane_normal[0]) + aabb_half[1] * glm::abs(plane_normal[1]) + aabb_half[2] * glm::abs(plane_normal[2]);
		float s = glm::dot(plane_normal, aabb_origin) - plane_d;
	
		return glm::abs(s) <= r;
	}

    /// position_aabb_plane retun value
    /// -1 - AABB is in the halfspace lying in negative plane normal direction 
    ///  0 - Plane is intersecting AABB
    ///  1 - AABB is in the halfspace lying in plane normal direction

    inline uint8 position_aabb_plane(const double3& aabb_origin, 
        const float3& aabb_half,
        const double3& plane_origin,
        const float3& plane_normal) 
    {
        const float3 aabb_pos(aabb_origin - plane_origin);

        const float mp = glm::dot(plane_normal, aabb_pos);
        const float np = glm::dot(plane_normal, aabb_half);

        if (mp + np < 0.0f) {
            return -1;
        }
        else if (mp - np < 0.0f) {
            return 0;
        }
        else {
            return 1;
        }
    }

    inline bool intersects_frustum_aabb(const double3& aabb_origin,
        const float3& aabb_half,
        const double3& frustum_origin,
        const float4* frustum_plane_normals,
        uint nplanes,
        bool include_partial) 
    {
        const float3 aabb_pos(aabb_origin - frustum_origin);

        for (uint i = 0; i < nplanes; i++) {
            const float3 n(frustum_plane_normals[i]);
            const float mp = glm::dot(n, aabb_pos) + frustum_plane_normals[i].w;
            const float np = glm::dot(glm::abs(n), aabb_half);
            if ((include_partial ? mp + np : mp - np) < 0.0f) {
                return false;
            }
        }

        return true;
    }

	inline bool intersects_triangle_aabb(const glm::vec3& triangle_a,
	                                     const glm::vec3& triangle_b,
	                                     const glm::vec3& triangle_c,
	                                     const glm::vec3& aabb_origin,
	                                     const glm::vec3& aabb_half)
	{
		glm::vec3 v0, v1, v2;
		float p0, p2, r;

		// Translate triangle as conceptually moving AABB to origin
		v0 = triangle_a - aabb_origin;
		v1 = triangle_b - aabb_origin;
		v2 = triangle_c - aabb_origin;

		// Compute edges of triangle
		glm::vec3 f0 = v1 - v0, f1 = v2 - v1, f2 = v0 - v2;
		// Test axes a00..a22 (category 3)


		// Test axis a00
		p0 = v0.z*v1.y - v0.y*v1.z;
		p2 = v2.z*(v1.y - v0.y) - v2.z*(v1.z - v0.z);
		r = aabb_half.y * glm::abs(f0.z) + aabb_half.z * glm::abs(f0.y);
		if (glm::max(-glm::max(p0, p2), glm::min(p0, p2)) > r) return false;
		// Test axis a01
		p0 = v0.y*(v1.z - v2.z) + v0.z*(v2.y - v1.y);
		p2 = v1.z*v2.y - v1.y*v2.z;
		r = aabb_half.y * glm::abs(f1.z) + aabb_half.z * glm::abs(f1.y);
		if (glm::max(-glm::max(p0, p2), glm::min(p0, p2)) > r) return false;
		// Test axis a02
		p0 = v0.y*v2.z - v0.z*v2.y;
		p2 = v1.y*(v2.z - v0.z) + v1.z*(v0.y - v2.y);
		r = aabb_half.y * glm::abs(f2.z) + aabb_half.z * glm::abs(f2.y);
		if (glm::max(-glm::max(p0, p2), glm::min(p0, p2)) > r) return false;

		// Test axis a10
		p0 = v0.x*v1.z - v0.z*v1.x;
		p2 = v2.x*(v1.z - v0.z) + v2.z*(v0.x - v1.x);
		r = aabb_half.x * glm::abs(f0.z) + aabb_half.z * glm::abs(f0.x);
		if (glm::max(-glm::max(p0, p2), glm::min(p0, p2)) > r) return false;
		// Test axis a11
		p0 = v0.x*(v2.z - v1.z) + v0.z*(v1.x - v2.x);
		p2 = v1.x*v2.z - v1.z*v2.x;
		r = aabb_half.x * glm::abs(f1.z) + aabb_half.z * glm::abs(f1.x);
		if (glm::max(-glm::max(p0, p2), glm::min(p0, p2)) > r) return false;
		// Test axis a12
		p0 = v0.z*v2.x - v0.x*v2.z;
		p2 = v1.x*(v0.z - v2.z) + v1.z*(v2.x - v0.x);
		r = aabb_half.x * glm::abs(f2.z) + aabb_half.z * glm::abs(f2.x);
		if (glm::max(-glm::max(p0, p2), glm::min(p0, p2)) > r) return false;

		// Test axis a20
		p0 = v0.y*v1.x - v0.x*v1.y;
		p2 = v2.x*(v0.y - v1.y) * v2.y*(v1.x - v0.x);
		r = aabb_half.x * glm::abs(f0.y) + aabb_half.y * glm::abs(f0.x);
		if (glm::max(-glm::max(p0, p2), glm::min(p0, p2)) > r) return false;
		// Test axis a21
		p0 = v0.x*(v1.y - v2.y) + v0.y*(v2.x - v1.x);
		p2 = v1.y*v2.x - v1.x*v2.y;
		r = aabb_half.x * glm::abs(f1.y) + aabb_half.y * glm::abs(f1.x);
		if (glm::max(-glm::max(p0, p2), glm::min(p0, p2)) > r) return false;
		// Test axis a22
		p0 = v0.x*v2.y - v0.y*v2.x;
		p2 = v1.x*(v2.y - v0.y) + v1.y*(v0.x - v2.x);
		r = aabb_half.x * glm::abs(f2.y) + aabb_half.y * glm::abs(f2.x);
		if (glm::max(-glm::max(p0, p2), glm::min(p0, p2)) > r) return false;

		if (glm::max(v0.x, glm::max(v1.x, v2.x)) < -aabb_half.x || glm::min(v0.x, glm::min(v1.x, v2.x)) > aabb_half.x) return false;

		if (glm::max(v0.y, glm::max(v1.y, v2.y)) < -aabb_half.y || glm::min(v0.y, glm::min(v1.y, v2.y)) > aabb_half.y) return false;

		if (glm::max(v0.z, glm::max(v1.z, v2.z)) < -aabb_half.z || glm::min(v0.z, glm::min(v1.z, v2.z)) > aabb_half.z) return false;
		
		glm::vec3 pn = glm::cross(f0, f1);
		float pd = glm::dot(pn, v0);
		return intersects_aabb_plane(glm::vec3(0),aabb_half,pd,pn );

	}
	
    template<typename T> bool intersects_aabb_aabb(const T& aabb1_origin,
        const T& aabb1_half,
        const T& aabb2_origin,
        const T& aabb2_half)
    {
        const T t = aabb2_origin - aabb1_origin;

        return glm::abs(t.x) <= (aabb1_half.x + aabb2_half.x) &&
            glm::abs(t.y) <= (aabb1_half.y + aabb2_half.y) &&
            glm::abs(t.z) <= (aabb1_half.z + aabb2_half.z);
    }

	inline bool intersects_aabb_obb(const glm::vec3& aabb_origin,
		const glm::vec3& aabb_half, 
		const glm::vec3& obb_origin, 
		const glm::vec3& obb_half, 
		const glm::mat3& obb_rot)
	{
		return false;
	}

	inline bool intersects_point_aabb(const glm::vec3 & point_p,
		const glm::vec3 & aabb_origin, 
		const glm::vec3 & aabb_half,
		float * dist)
	{
		const glm::vec3 point = point_p - aabb_origin;
		float  dmin = 0;

		for (int i = 0; i < 3; i++) {
			if (point[i] < -aabb_half[i]) {
				dmin += glm::pow(point[i] + aabb_half[i], 2.0f);
			}
			else if (point[i] > aabb_half[i]) {
				dmin += glm::pow(point[i] - aabb_half[i], 2.0f);
			}
		}

		if (dmin == 0) {
			if (dist) {
				*dist = 0;
			}
			return true;
		}
		else {
			return false;
		}
	}

    /*
    Christer Ericson’s Real-time Collision Detection
    Page 136, 5.1.5 Closest Point on Triangle to Point
    */

    inline float distance_point_trinagle_sqr(const glm::vec3& point_p, const
        glm::vec3& triangle_a,
        const glm::vec3& triangle_b,
        const glm::vec3& triangle_c,
        float& u,
        float& v,
        float& w,
        float3& contact_point)
	{
		const glm::vec3 ab(triangle_b - triangle_a);
		const glm::vec3 ac(triangle_c - triangle_a);
		const glm::vec3 bc(triangle_c - triangle_b);
		const glm::vec3 ap(point_p - triangle_a);
		const glm::vec3 bp(point_p - triangle_b);
		const glm::vec3 cp(point_p - triangle_c);


		const float d1 = glm::dot(ab, ap);   // === snom
		const float d2 = glm::dot(ac, ap);   // === tnom   
		const float d3 = glm::dot(ab, bp);  // === -sdenom
		const float d4 = glm::dot(ac, bp); 
		const float d5 = glm::dot(ab, cp); 
		const float d6 = glm::dot(ac, cp); // === -tdenom
		const float unom = d4 - d3;
		const float udenom = d5 - d6;

		if (d1 <= 0.0f && d2 <= 0.0f) {
            u = 1.0f;
            v = 0.0f;
            w = 0.0f;
            contact_point = triangle_a;
			return glm::dot(ap,ap);
		}

		if (d3 >= 0.0f && d4 <= d3) {
            u = 0.0f;
            v = 1.0f;
            w = 0.0f;
            contact_point = triangle_b;
			return glm::dot(bp, bp);
		}

		if (d6 >= 0.0f && d5 <= d5) {
            u = 0.0f;
            v = 0.0f;
            w = 1.0f;
            contact_point = triangle_c;
            return glm::dot(cp, cp);
		}

        const float vc = d1*d4 - d3*d2;

		if (vc <= 0.0f && d1 >= 0.0f && d3 < 0.0f) {
            v = d1 / (d1 - d3);
            u = 1.0f - v;
            w = 0.0f;
            contact_point = triangle_a + v*ab;
			const glm::vec3 pcp = point_p - cp;
			return glm::dot(pcp, pcp);
		}

        const float vb = d5*d2 - d1*d6;

		if (vb < 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
            w = d2 / (d2 - d6);
            u = 1.0f - w;
            v = 0.0f;
            contact_point = triangle_b + w*ac;
            const glm::vec3 pcp = point_p - cp;
            return glm::dot(pcp, pcp);
		}

        const float va = d3*d6 - d5*d4;

        if (va <= 0.0f && d4 >= d3 && d5 >= d6) {
            w = (d4 - d3) / (d4 - d3 + d5 - d6);
            v = 1.0f - w;
            u = 0.0f;
            contact_point = triangle_b + w*bc;
            const glm::vec3 pcp = point_p - cp;
            return glm::dot(pcp, pcp);
        }



        const float denom = 1.0f / (va + vb + vc);
        v = vb * denom;
        w = vc * denom;
        u = 1.0f - v - w;
        
        contact_point = triangle_a + ab*v + ac*w;
        const glm::vec3 pcp = point_p - contact_point;
        return glm::dot(pcp, pcp);
	}

    /*
        Christer Ericson’s Real-time Collision Detection
        Page 129, 5.1.2.1 Distance of Point to Segment
    */

	inline float distance_point_segment_sqr(const glm::vec3 & point_p, const  glm::vec3 & segment_a, const glm::vec3 & segment_b)
	{
        const glm::vec3 ab = segment_b - segment_a;
        const glm::vec3 ap = point_p - segment_a;
        const glm::vec3 bp = point_p - segment_b;
		float e = glm::dot(ap, ab);
		if (e <= 0.0f) return glm::dot(ap, ap);
		float f = glm::dot(ab, ab);
		if (e >= f) return glm::dot(bp, bp);
		return glm::dot(ap, ap) - (e * e) / f;
	}

    /*
    Christer Ericson’s Real-time Collision Detection
    Page 148, 5.1.9 Closest Points of Two Line Segments
    */

	inline float distance_segment_segment_sqr(const glm::vec3 & segment_pt1, 
        const glm::vec3 & segment_dir1, 
        const glm::vec3 & segment_pt2, 
        const glm::vec3 & segment_dir2, 
        float& s, 
        float& t)
	{
		
		const glm::vec3 r = segment_pt1 - segment_pt2;
		float a = glm::dot(segment_dir1, segment_dir1);
		float e = glm::dot(segment_dir2, segment_dir2);
		float f = glm::dot(segment_dir2, r);
		
		if (a <= FLOAT_EPS && e <= FLOAT_EPS) {
			s = t = 0.0f;
			return glm::dot(r, r);
		}

		if (a <= FLOAT_EPS) {
			s = 0.0f;
			t = f / e; // s = 0 => t = (b*s + f) / e = f / e
			t = glm::clamp(t, 0.0f, 1.0f);
		}
		else {
			float c = glm::dot(segment_dir1, r);
			if (e <= FLOAT_EPS) {
				// Second segment degenerates into a point
				t = 0.0f;
				s = glm::clamp(-c / a, 0.0f, 1.0f); // t = 0 => s = (b*t - c) / a = -c / a
			}
			else {
				// The general nondegenerate case starts here
				float b = glm::dot(segment_dir1, segment_dir2);
				float denom = a*e - b*b; // Always nonnegative
										 // If segments not parallel, compute closest point on L1 to L2 and
										 // clamp to segment S1. Else pick arbitrary s (here 0)
				if (denom != 0.0f) {
					s = glm::clamp((b*f - c*e) / denom, 0.0f, 1.0f);
				}
				else s = 0.0f;
				// Compute point on L2 closest to S1(s) using
				// t = Dot((P1 + D1*s) - P2,D2) / Dot(D2,D2) = (b*s + f) / e
				t = (b*s + f) / e;
				// If t in [0,1] done. Else clamp t, recompute s for the new value
				// of t using s = Dot((P2 + D2*t) - P1,D1) / Dot(D1,D1)= (t*b - c) / a
				// and clamp s to [0, 1]
				if (t < 0.0f) {
					t = 0.0f;
					s = glm::clamp(-c / a, 0.0f, 1.0f);
				}
				else if (t > 1.0f) {
					t = 1.0f;
					s = glm::clamp((b - c) / a, 0.0f, 1.0f);
				}
			}
		}

		const glm::vec3 c1 = segment_pt1 + segment_dir1 * s;
		const glm::vec3 c2 = segment_pt2 + segment_dir2 * t;
        const glm::vec3 c12 = c2 - c1;
		return glm::dot(c12,c12);
	}

    /*
    Christer Ericson’s Real-time Collision Detection
    Page 153, 5.1.10 Closest Points of a Line Segment and a Triangle
    */

    inline float distance_segment_triangle_sqr(const float3& p, 
        const float3& q,
        const float3& a, 
        const float3& b, 
        const float3& c,
        float& t, 
        float& u, 
        float& v, 
        float& w)
    {
        // 1. check if pq intersects abc plane in triangle interior. if yes, we are done.
        // 2. check if p and q projects inside of abc. if yes, get the minimum and we are done
        // 3. calculate segment vs. each abc's edge and choose minimum. also check result against p or q if 
        // they projection lies within triangle. and we are finally done.

        const float3 pq = q - p;
        const float3 ab = b - a;
        const float3 ac = c - a;
        const float3 bc = c - b;
        const float3 ap = p - a;
        const float3 aq = q - a;
        const float3 tn = glm::normalize(glm::cross(ab,ac));

        const float d00 = glm::dot(ab, ab);
        const float d01 = glm::dot(ab, ac);
        const float d11 = glm::dot(ac, ac);
        const float bdenom_inv = 1.0f / ((d00 * d11) - (d01*d01));

        /// 1.

        const float dist1 = glm::dot(ap, tn); // signed distance of p from triangle plane
        const float dist2 = glm::dot(aq, tn); // signed distance of q from triangle plane
        const float dist1sq = dist1 * dist1;
        const float dist2sq = dist2 * dist2;


        if (dist1 * dist2 < 0.f) // sign mismatch so it intersects 
        {
            const float t0 = glm::dot(ap, tn) / glm::dot(pq, tn);
            const float3 x = p - pq*t0;
            const float3 ax = x - a;
            const float d20 = glm::dot(ax,ab);
            const float d21 = glm::dot(ax, ac);
            const float v0 = (d11 * d20 - d01*d21) * bdenom_inv;
            const float w0 = (d00 * d21 - d01*d20) * bdenom_inv;
            
            if (is_valid_barycentric_coord(v0, w0)) {
                v = v0;
                w = w0;
                u = 1 - v - w;
                t = -t0;
                return 0.f;
            }
        }
        
        /// 2.
        // projection of p and q to triangle plane
        const float3 proj_p = p - tn*dist1;
        const float3 proj_q = q - tn*dist2;
        const float3 pax = proj_p - a;
        const float3 qax = proj_q - a;
        const float pd20 = glm::dot(pax, ab);
        const float pd21 = glm::dot(pax, ac);
        const float qd20 = glm::dot(qax, ab);
        const float qd21 = glm::dot(qax, ac);
        const float pv0 = (d11*pd20 - d01*pd21) * bdenom_inv;
        const float pw0 = (d00 * pd21 - d01*pd20) * bdenom_inv;
        const float qv0 = (d11*qd20 - d01*qd21) * bdenom_inv;
        const float qw0 = (d00 * qd21 - d01*qd20) * bdenom_inv;

        const bool p_in = is_valid_barycentric_coord(pv0, pw0);
        const bool q_in = is_valid_barycentric_coord(qv0, qw0);

        if (p_in && q_in) {
            // p and q projection are inside of triangle abc
            bool p_closer = dist1sq < dist2sq;
            v = (p_closer) ? pv0 : qv0;
            w = (p_closer) ? pw0 : qw0;
            u = 1 - v - w;
            t = (p_closer) ? 0.f : 1.f;
            return (p_closer) ? dist1sq : dist2sq;
        }
        else {
            float res_dist;

            float abs, abt, bcs, bct, acs, act,abd,bcd,acd;
            abd = distance_segment_segment_sqr(p, pq, a, ab, abs, abt);
            bcd = distance_segment_segment_sqr(p, pq, b, bc, bcs, bct);
            acd = distance_segment_segment_sqr(p, pq, a, ac, acs, act);

            if (abd < bcd && abd < acd) {
                // closest point is on edge AB
                t = abs;
                u = 1.f - abt;
                v = abt;
                w = 0.f;
                res_dist = abd;
            }
            else if (bcd < acd) {
                // closest point is on edge BC
                t = bcs;
                u = 0.f;
                v = 1.f - bct;
                w = bct;
                res_dist = bcd;
            }
            else {
                // closest point is on edge AC
                t = acs;
                u = 1.f - act;
                v = 0.f;
                w = act;
                res_dist = acd;
            }

            if (p_in && dist1sq < res_dist) {
                t = 0.f;
                v = pv0 ;
                w = pw0;
                u = 1.f - v - w;
                return dist1sq;
            }

            if (q_in && dist2sq < res_dist) {
                t = 1.f;
                v = qv0;
                w = qw0;
                u = 1.f - v - w;
                return dist2sq;
            }

            return res_dist;
        }
    }
    //////////////////////////////////////////////////////////////

	inline bool is_edge_convex(const glm::vec3 & v1, const glm::vec3 & v2, const glm::vec3 & v3, const glm::vec3 & v4)
	{
       /* float3 e01(0.0131664276f, 0.137915611f, 0.261226654f);
        float3 e12(-0.304033279f, -0.0884456635f, -0.276754379f);
        float3 e20(0.290866852f, -0.0494699478f, 0.0155277252f);

        if (v1 - v3 == e01) {
            e01 == e12;
        }

        if (v1 - v3 == e12) {
            e01 == e12;
        }

        if (v1 - v3 == e20) {
            e01 == e12;
        }
        */
        
        static const float threshold_angle_cos = 0.996f;

        const glm::vec3 n1 = glm::normalize(glm::cross(v2 - v1, v3 - v1));
		const glm::vec3 n2 = glm::normalize(glm::cross(v3 - v1, v4 - v1));
		const float cos_n12 = glm::clamp(glm::dot(n1, n2), -1.0f, 1.0f);


		if (glm::dot(glm::normalize(v4 - v2), n1) < 0) {
			if (cos_n12 <= threshold_angle_cos) {
				return true;
			}
		}
		else if (cos_n12 < -0.999) {
			return true;
		}

		return false;
	}

    /*
    [Möller97a] Möller, Tomas. Ben Trumbore. “Fast, Minimum-storage Ray/
    Triangle Intersection,” Journal of Graphics Tools, vol. 2, no. 1, pp. 21–28, 1997.
    http://www.graphics.cornell.edu/pubs/1997/MT97.html
    */

    inline bool intersects_ray_triangle(const float3 & ray_start,
        const float3 & ray_dir,
        const float3 & triangle_a,
        const float3 & triangle_b,
        const float3 & triangle_c,
        float & res_u,
        float & res_v,
        float & res_w,
        float & res_t,
        float margin = 0.001f) 
    {
        const float3 e1(triangle_b - triangle_a);
        const float3 e2(triangle_c - triangle_a);
        const float3 pvec(glm::cross(ray_dir, e2));
        const float det = glm::dot(e1, pvec);

        if (det > -FLOAT_EPS && det < FLOAT_EPS) {
            return false;
        }

        const float3 tvec(ray_start - triangle_a);
        res_v = glm::dot(tvec, pvec);
        if (res_v < -margin || res_v > det+ margin) {
            return false;
        }

        const float3 qvec(glm::cross(tvec, e1));
        res_w = glm::dot(ray_dir, qvec);
        if (res_w < -margin || res_v + res_w > det+ margin) {
            return false;
        }

        const float inv_det = 1.f / det;

        res_t = glm::dot(e2, qvec) * inv_det;
        res_v *= inv_det;
        res_w *= inv_det;
        res_u = 1 - res_v - res_w;
        return true;
    }

    inline float3 transform_point_with_dq(const quat & rot, const quat & dq, const float3 & p)
    {
        const float3 r_xyz(rot.x, rot.y, rot.z);
        const float3 dq_xyz(dq.x, dq.y, dq.z);
        return p + 2.f * glm::cross(r_xyz, glm::cross(r_xyz, p) + rot.w * p)
            + 2.f * (rot.w * dq_xyz - dq.w * r_xyz + glm::cross(r_xyz, dq_xyz));
    }

    inline float4 norm_uchar4_to_float4(const uchar4 & norm) {
        return float4(static_cast<float>(norm.x) / 255.f,
            static_cast<float>(norm.y) / 255.f,
            static_cast<float>(norm.z) / 255.f,
            static_cast<float>(norm.w) / 255.f);
    }

    inline void compute_deform_dq(const float4** bones_rot_ptrs, const float4** bones_dq_ptrs,const float4& weights,float4 * res_rot, float4* res_dq) {
        *res_rot = *bones_rot_ptrs[0];
        *res_dq = *bones_dq_ptrs[0];
        const float4 q(*res_rot);
        *res_rot *= weights.x;
        *res_dq *= weights.x;

        for (int i = 1; i < 4; i++) {
            const float4 tmp_rot = *bones_rot_ptrs[i];
            const float4 tmp_dq = *bones_dq_ptrs[i];
            
           * res_rot += tmp_rot * (glm::dot(q, tmp_rot) < 0.0 ? -weights[i] : weights[i]);
           * res_dq += tmp_dq * (glm::dot(q, tmp_rot) < 0.0 ? -weights[i] : weights[i]);
        }

        const float rot_len = glm::inversesqrt(glm::dot(*res_rot, *res_rot));
        *res_rot *= rot_len;
        *res_dq *= rot_len;
    }
}