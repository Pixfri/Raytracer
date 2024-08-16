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

        DescriptorAllocatorGrowable m_GlobalDescriptorAllocator;

        VkDescriptorSet m_DrawImageDescriptors;
        VkDescriptorSetLayout m_DrawImageDescriptorLayout;

    public:
        AllocatedImage DrawImage;
        VkExtent2D DrawExtent;
        float RenderScale = 1.0f;
        
        VulkanRenderer(const Window& window, const DebugLevel& debugLevel);
        ~VulkanRenderer();

        VulkanRenderer(const VulkanRenderer&) = delete;
        VulkanRenderer(VulkanRenderer&&) = delete;

        VulkanRenderer& operator=(const VulkanRenderer&) = delete;
        VulkanRenderer& operator=(VulkanRenderer&&) = delete;

        void PlanDescriptorPoolsDeletion(DescriptorAllocatorGrowable& allocator);
        void PlanDeletion(std::function<void()>&& deletor);

        VkCommandBuffer BeginCommandBuffer(const Window& window);
        void EndCommandBuffer(Window& window);

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

        [[nodiscard]] FrameData& GetCurrentFrame() {
            return m_Frames[m_FrameNumber % g_FrameOverlap];
        }

        void RecreateSwapchain(Window& window);
    };

#include <Raytracer/Renderer/VulkanRenderer.inl>
}
