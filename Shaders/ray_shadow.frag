// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#version 460
#extension GL_EXT_ray_query : enable
#extension GL_GOOGLE_include_directive : enable

#include "input_structures.glsl"

layout (location = 0) in vec4 VertexPos;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec4 ScenePosition; // Scene with respect to BVH coordinates.

layout (location = 0) out vec4 FragColor;

/*
 * Calculate ambien occlusion.
 */
float calculateAmbientOcclusion(vec3 objectPoint, vec3 objectNormal) {
    const float ao_mut = 1;
    uint max_ao_each = 3;
    uint max_ao = max_ao_each * max_ao_each;
    const float max_dist = 2;
    const float tmin = 0.01, tmax = max_dist;
    float accumulated_ao = 0.f;
    vec3 u = abs(dot(objectNormal, vec3(0, 0, 1))) > 0.9 ? cross(objectNormal, vec3(1, 0, 0)) : cross(objectNormal, vec3(0, 0, 1));
    vec3 v = cross(objectNormal, u);
    float accumulated_factor = 0;
    for (uint j = 0; j < max_ao_each; ++j) {
        float phi = 0.5*(-3.14159 * (float(j + 1) / float(max_ao_each + 2)));
        for (uint k = 0; k < max_ao_each; ++k) {
            float theta = 0.5*(-3.14159 * (float(k + 1) / float(max_ao_each + 2)));
            float x = cos(phi) * sin(theta);
            float y = sin(phi) * sin(theta);
            float z = cos(theta);
            vec3 direction = x * u + y * v + z * objectNormal;

            rayQueryEXT query;
            rayQueryInitializeEXT(query, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, objectPoint, tmin, direction.xyz, tmax);
            rayQueryProceedEXT(query);
            float dist = max_dist;
            if (rayQueryGetIntersectionTypeEXT(query, true) != gl_RayQueryCommittedIntersectionNoneEXT) {
                dist = rayQueryGetIntersectionTEXT(query, true);
            }
            float ao = min(dist, max_dist);
            float factor = 0.2 + 0.8 * z * z;
            accumulated_factor += factor;
            accumulated_ao += ao * factor;
        }
    }
    accumulated_ao /= (max_dist * accumulated_factor);
    accumulated_ao *= accumulated_ao;
    accumulated_ao = max(min((accumulated_ao) * ao_mut, 1), 0);
    return accumulated_ao;
}

/*
 * Apply ray tracing to determine whether the point intersects light.
 */
bool intersectsLight(vec3 lightOrigin, vec3 pos) {
    const float tmin = 0.01, tmax = 1000;
    const vec3 direction = lightOrigin - pos;

    rayQueryEXT query;

    // The following runs the actual ray query.
    // For performance, use gl_RayFlagsTerminateOnFirstHitEXT, since we only need to know
    // wheter an intersection exists, and not necessarily any particular intersection.
    rayQueryInitializeEXT(query, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, pos, tmin, direction.xyz, 1.0);
    // The following is the canonical way of using ray queries from the fragment shader when
    // there's more than one bounce or hit to traverse:
    // while(rayQueryProceedEXT(query)) { }
    // Since the flag gl_RayFlagsTerminateOnFirstHitEXT is set, there will never be a bounce and no need for an expensive while loop.
    rayQueryProceedEXT(query);
    if (rayQueryGetIntersectionTypeEXT(query, true) != gl_RayQueryCommittedIntersectionNoneEXT) {
        return true;
    }

    return false;
}

void main() {
    const float ao = calculateAmbientOcclusion(ScenePosition.xyz, VertexNormal);
    const vec4 lighting = intersectsLight(globalUniform.lightPosition, ScenePosition.xyz) ? vec4(0.2, 0.2, 0.2, 1) : vec4(1, 1, 1, 1);
    FragColor = lighting * vec4(ao * vec3(1, 1, 1), 1);
}