// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <Raytracer/rtpch.hpp>

#include <Raytracer/Renderer/VulkanDescriptors.hpp>
#include <Raytracer/Renderer/VulkanRenderer.hpp>

#include <glm/glm.hpp>

namespace Raytracer {
    struct RaytracingPushConstants {
        glm::vec4 Data1;
        glm::vec4 Data2;
        glm::vec4 Data3;
        glm::vec4 Data4;
    };

    struct RaytracingShader {
        VkPipeline Pipeline;
        VkPipelineLayout Layout;

        RaytracingPushConstants Data;
    };
    
    class ApplicationRenderer {
    public:
        explicit ApplicationRenderer(Renderer::VulkanRenderer* renderer);
        ~ApplicationRenderer();
        
        ApplicationRenderer(const ApplicationRenderer&) = delete;
        ApplicationRenderer(ApplicationRenderer&&) = delete;
        
        ApplicationRenderer& operator=(const ApplicationRenderer&) = delete;
        ApplicationRenderer& operator=(ApplicationRenderer&&) = delete;

        void Raytrace(VkCommandBuffer commandBuffer) const;

    private:
        Renderer::VulkanRenderer* m_Renderer;
        
        DeletionQueue m_DeletionQueue;
        
        Renderer::DescriptorAllocatorGrowable m_DescriptorAllocator;

        VkDescriptorSet m_DrawImageDescriptors;
        VkDescriptorSetLayout m_DrawImageDescriptorLayout;

        RaytracingShader m_RaytracingShader;
        
        void InitializeDescriptors();
        void InitializeRaytracePipeline();
    };
}

#include <Raytracer/ApplicationRenderer.inl>
