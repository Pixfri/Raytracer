// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#version 460
#extension GL_EXT_ray_query : enable
#extension GL_GOOGLE_include_directive : enable

#include "input_structures.glsl"

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

layout (location = 0) out vec4 VertexPos;
layout (location = 1) out vec3 VertexNormal;
layout (location = 2) out vec4 ScenePosition; // Scene with respect to BVH coordinates.

void main() {
    // We want to be able to perform ray tracing, so don't apply any matrix to ScenePosition.
    ScenePosition = vec4(position, 1);

    VertexPos = globalUniform.view * vec4(position, 1);

    VertexNormal = normal;

    gl_Position = globalUniform.proj * globalUniform.view * vec4(position, 1.0);
}