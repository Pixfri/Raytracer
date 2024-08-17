// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE


#include <Raytracer/RaytracerApp/RayQueryRenderer.hpp>

namespace Raytracer {

    RayQueryRenderer::RayQueryRenderer(Renderer::VulkanRenderer* renderer, Camera& camera) : m_Renderer(
        renderer) {
    }

    RayQueryRenderer::~RayQueryRenderer() {
        vkDeviceWaitIdle(m_Renderer->GetDevice().GetDevice());

        m_DeletionQueue.Flush();
    }
}
