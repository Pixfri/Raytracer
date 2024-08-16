// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <Raytracer/Renderer/VulkanDescriptors.hpp>
#include <Raytracer/Renderer/VulkanWrapper/Swapchain.hpp>

#include <Raytracer/Core/Window.hpp>

#include <VkBootstrap.h>

namespace Raytracer::Renderer {

    constexpr u32 g_FrameOverlap = 2;

    struct FrameData {
        VkCommandPool CommandPool;
        VkCommandBuffer MainCommandBuffer;

        VkSemaphore SwapchainSemaphore, RenderSemaphore;
        VkFence RenderFence;

        u32 SwapchainImageIndex;

        DeletionQueue DeletionQueue;
        DescriptorAllocatorGrowable FrameDescriptors;
    };

    struct ComputePushConstants {
        glm::vec4 Data1;
        glm::vec4 Data2;
        glm::vec4 Data3;
        glm::vec4 Data4;
    };

    struct ComputeShader {
        VkPipeline Pipeline;
        VkPipelineLayout Layout;

        ComputePushConstants Data;
    };

    class VulkanRenderer {
        bool m_RendererInitialized = false;

        DeletionQueue m_MainDeletionQueue;

        std::unique_ptr<VulkanWrapper::Instance> m_Instance;
        std::unique_ptr<VulkanWrapper::Device> m_Device;

        VmaAllocator m_Allocator;

        std::unique_ptr<VulkanWrapper::Swapchain> m_Swapchain;
        bool m_SwapchainResizeRequired{false};

        FrameData m_Frames[g_FrameOverlap];
        i32 m_FrameNumber = 0;

        VkFence m_ImmediateFence;
        VkCommandBuffer m_ImmediateCommandBuffer;
        VkCommandPool m_ImmediateCommandPool;

        AllocatedImage m_DrawImage;
        VkExtent2D m_DrawExtent;
        float m_RenderScale = 1.0f;

        DescriptorAllocatorGrowable m_GlobalDescriptorAllocator;

        VkDescriptorSet m_DrawImageDescriptors;
        VkDescriptorSetLayout m_DrawImageDescriptorLayout;

        VkPipelineLayout m_ComputePipelineLayout;

        ComputeShader m_Raytrace;

        VkDescriptorSetLayout m_SingleImageDescriptorLayout;

    public:
        
        VulkanRenderer(const Window& window, const DebugLevel& debugLevel);
        ~VulkanRenderer();

        VulkanRenderer(const VulkanRenderer&) = delete;
        VulkanRenderer(VulkanRenderer&&) = delete;

        VulkanRenderer& operator=(const VulkanRenderer&) = delete;
        VulkanRenderer& operator=(VulkanRenderer&&) = delete;

        void PlanDescriptorPoolsDeletion(DescriptorAllocatorGrowable& allocator);
        void PlanDeletion(std::function<void()>&& deletor);
        void Draw(Window& window);

        void ImmediateSubmit(const std::function<void(VkCommandBuffer commandBuffer)>& function) const;

        [[nodiscard]] inline VulkanWrapper::Instance& GetInstance() const;
        [[nodiscard]] inline VulkanWrapper::Device& GetDevice() const;
        [[nodiscard]] inline VmaAllocator GetAllocator() const;
        [[nodiscard]] inline VkFormat GetDrawImageFormat() const;

    private:
        void InitializeVulkan(const Window& window, const DebugLevel& debugLevel);
        void InitializeSwapchain(const Window& window);
        void InitializeCommands();
        void InitializeFramesCommandBuffers();
        void InitializeImmediateCommandBuffer();
        void InitializeSynchronisationPrimitives();
        void InitializeDescriptors();
        void InitializePipelines();
        void InitializeComputePipelines();

        void Raytrace(VkCommandBuffer commandBuffer) const;

        [[nodiscard]] FrameData& GetCurrentFrame() {
            return m_Frames[m_FrameNumber % g_FrameOverlap];
        }

        void RecreateSwapchain(Window& window);
    };

#include <Raytracer/Renderer/VulkanRenderer.inl>
}
