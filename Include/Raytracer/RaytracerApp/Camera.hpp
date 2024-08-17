// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <Raytracer/rtpch.hpp>

#include <glm/glm.hpp>

#include <vulkan/vulkan.h>

namespace Raytracer {
    class Camera {
        f32 m_LastMouseX = 960;
        f32 m_LastMouseY = 540;

        bool m_EnableMouse = false;
    
    public:
        bool Updated = false;

        glm::vec3 Velocity;
        glm::vec3 Position;
        // Vertical rotation
        f32 Pitch = {0.f};
        // Horizontal rotation
        f32 Yaw = {0.f};
        f32 Fov = {70.f};

        glm::mat4 GetViewMatrix();
        glm::mat4 GetProjectionMatrix(VkExtent2D viewportExtent) const;
        glm::mat4 GetRotationMatrix() const;

        void OnKeyDown(i32 keyScancode, f64 deltaTime, f32 speed);
        void OnKeyUp(i32 keyScancode);
        void OnMouseMovement(f32 mouseX, f32 mouseY, f32 sensitivity, f64 deltaTime);

        void Update();
    };
}

#include <Raytracer/RaytracerApp/Camera.inl>
