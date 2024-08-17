// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <Raytracer/rtpch.hpp>

#include <Raytracer/Renderer/VulkanDescriptors.hpp>
#include <Raytracer/Renderer/VulkanRenderer.hpp>

#include <Raytracer/RaytracerApp/Camera.hpp>

namespace Raytracer {
    
    class RayQueryRenderer {
    public:        
        explicit RayQueryRenderer(Renderer::VulkanRenderer* renderer, Camera& camera);
        ~RayQueryRenderer();
        
        RayQueryRenderer(const RayQueryRenderer&) = delete;
        RayQueryRenderer(RayQueryRenderer&&) = delete;
        
        RayQueryRenderer& operator=(const RayQueryRenderer&) = delete;
        RayQueryRenderer& operator=(RayQueryRenderer&&) = delete;
        

    private:
        Renderer::VulkanRenderer* m_Renderer;
        
        DeletionQueue m_DeletionQueue;

        Renderer::DescriptorAllocatorGrowable m_DescriptorAllocator;
        
    };
}

#include <Raytracer/RaytracerApp/RayQueryRenderer.inl>
