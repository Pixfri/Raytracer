// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Raytracer/RaytracerApp/Camera.hpp>

#include <Raytracer/Core/Window.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Raytracer {
    glm::mat4 Camera::GetViewMatrix() {
        // To create a correct model view, the world needs to be moved in the opposite direction of the camera.
        const glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), Position);
        const glm::mat4 cameraRotation = GetRotationMatrix();
        Updated = false;

        return cameraTranslation * cameraRotation;
    }

    glm::mat4 Camera::GetProjectionMatrix(const VkExtent2D viewportExtent) const {
        return glm::perspective(
            Fov, static_cast<f32>(viewportExtent.width) / static_cast<f32>(viewportExtent.height), 0.1f, 10000.f);
    }

    glm::mat4 Camera::GetRotationMatrix() const {
        const glm::quat pitchRotation = glm::angleAxis(Pitch, glm::vec3{1.f, 0.f, 0.f});
        const glm::quat yawRotation = glm::angleAxis(Yaw, glm::vec3(0.f, 1.f, 0.f));

        return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
    }

    void Camera::OnKeyDown(const i32 keyScancode, const f64 deltaTime, const f32 speed) {
        // Camera movement
        if (keyScancode == static_cast<i32>(Keys::W)) {
            Velocity.z = -speed * deltaTime;
            Updated = true;
        }

        if (keyScancode == static_cast<i32>(Keys::S)) {
            Velocity.z = speed * deltaTime;
            Updated = true;
        }

        if (keyScancode == static_cast<i32>(Keys::A)) {
            Velocity.x = -speed * deltaTime;
            Updated = true;
        }

        if (keyScancode == static_cast<i32>(Keys::D)) {
            Velocity.x = speed * deltaTime;
            Updated = true;
        }

        if (keyScancode == static_cast<i32>(Keys::Q)) {
            m_EnableMouse = true;
        }
    }

    void Camera::OnKeyUp(const i32 keyScancode) {
        if (keyScancode == static_cast<i32>(Keys::W)) {
            Velocity.z = 0;
        }

        if (keyScancode == static_cast<i32>(Keys::S)) {
            Velocity.z = 0;
        }

        if (keyScancode == static_cast<i32>(Keys::A)) {
            Velocity.x = 0;
        }

        if (keyScancode == static_cast<i32>(Keys::D)) {
            Velocity.x = 0;
        }

        if (keyScancode == static_cast<i32>(Keys::Q)) {
            m_EnableMouse = false;
        }
    }

    void Camera::OnMouseMovement(const f32 mouseX, const f32 mouseY, const f32 sensitivity, const f64 deltaTime) {
        f32 xOffset = m_LastMouseX - mouseX;
        f32 yOffset = m_LastMouseY - mouseY;
        m_LastMouseX = mouseX;
        m_LastMouseY = mouseY;

        if (m_EnableMouse) {
            xOffset *= (sensitivity * deltaTime);
            yOffset *= (sensitivity * deltaTime);

            Yaw += xOffset;
            Pitch += yOffset;
        }
        Updated = true;
    }

    void Camera::Update() {
        const glm::mat4 cameraRotation = GetRotationMatrix();
        Position += glm::vec3(cameraRotation * glm::vec4(Velocity, 0.f));
    }
}
