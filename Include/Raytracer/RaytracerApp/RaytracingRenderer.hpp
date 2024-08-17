// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <Raytracer/rtpch.hpp>

#include <Raytracer/Renderer/VulkanDescriptors.hpp>
#include <Raytracer/Renderer/VulkanRenderer.hpp>

#include <glm/glm.hpp>

#include <VkBootstrap.h>

namespace Raytracer {
    // Holds data for a scratch buffer used as a temporary storage during acceleration structure builds.
    struct ScratchBuffer {
        VkDeviceAddress DeviceAddress;
        Renderer::AllocatedBuffer Buffer;
    };

    // Wraps all data required for an acceleration structure.
    struct AccelerationStructure {
        VkAccelerationStructureKHR Handle;
        VkDeviceAddress DeviceAddress;
        Renderer::AllocatedBuffer Buffer;
    };
    
    class RaytracingRenderer {
    public:
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR RayTracingPipelineProperties{};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR AccelerationStructureFeatures{};

        AccelerationStructure BottomLevelAccelerationStructure{};
        AccelerationStructure TopLevelAccelerationStructure{};

        Renderer::AllocatedBuffer VertexBuffer{};
        Renderer::AllocatedBuffer IndexBuffer{};
        u32 IndexCount{0};
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> ShaderGroups{};

        Renderer::AllocatedBuffer RayGenShaderBindingTable{};
        Renderer::AllocatedBuffer MissShaderBindingTable{};
        Renderer::AllocatedBuffer HitShaderBindingTable{};

        struct UniformData {
            glm::mat4 ViewInverse;
            glm::mat4 ProjInverse;
        } Uniforms{};
        Renderer::AllocatedBuffer UniformBuffer{};

        VkPipeline Pipeline = VK_NULL_HANDLE;
        VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
        
        explicit RaytracingRenderer(Renderer::VulkanRenderer* renderer);
        ~RaytracingRenderer();
        
        RaytracingRenderer(const RaytracingRenderer&) = delete;
        RaytracingRenderer(RaytracingRenderer&&) = delete;
        
        RaytracingRenderer& operator=(const RaytracingRenderer&) = delete;
        RaytracingRenderer& operator=(RaytracingRenderer&&) = delete;

    private:
        Renderer::VulkanRenderer* m_Renderer;
        
        DeletionQueue m_DeletionQueue;
        
        Renderer::DescriptorAllocatorGrowable m_DescriptorAllocator;

        void CreateBottomLevelAccelerationStructure();
        void CreateTopLevelAccelerationStructure();
        void DeleteAccelerationStructure(const AccelerationStructure& accelerationStructure) const;

        ScratchBuffer CreateScratchBuffer(VkDeviceSize size) const;
        void DeleteScratchBuffer(const ScratchBuffer& scratchBuffer) const;
        
        VkDeviceAddress GetBufferDeviceAddress(VkBuffer buffer) const;
    };
}

#include <Raytracer/RaytracerApp/RaytracingRenderer.inl>
