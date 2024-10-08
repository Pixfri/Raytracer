// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Raytracer/Renderer/VulkanWrapper/Swapchain.hpp>

#include <Raytracer/Renderer/VulkanRenderer.hpp>

namespace Raytracer::Renderer::VulkanWrapper {
    Swapchain::Swapchain(const Window& window, const Instance& instance, const Device& device) {
        Log::RtTrace("Creating Vulkan swapchain...");
        vkb::SwapchainBuilder swapchainBuilder{
            device.GetPhysicalDevice(), device.GetDevice(), instance.GetSurface()
        };

        m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

        vkb::Swapchain vkbSwapchain = swapchainBuilder
                                      .set_desired_format(VkSurfaceFormatKHR{
                                          .format = m_SwapchainImageFormat,
                                          .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR
                                      })
                                      .set_desired_present_mode(
                                          window.VSyncEnabled()
                                              ? VK_PRESENT_MODE_FIFO_KHR
                                              : VK_PRESENT_MODE_MAILBOX_KHR)
                                      .set_desired_extent(window.GetWidth(), window.GetHeight())
                                      .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                      .build()
                                      .value();

        Log::RtTrace("Vulkan swapchain created.");
        m_SwapchainExtent = vkbSwapchain.extent;
        m_Swapchain = vkbSwapchain.swapchain;
        m_SwapchainImages = vkbSwapchain.get_images().value();
        m_SwapchainImageViews = vkbSwapchain.get_image_views().value();

        m_DeletionQueue.PushFunction([&]() {
            Log::RtTrace("Destroying Vulkan swapchain.");
            vkDestroySwapchainKHR(device.GetDevice(), m_Swapchain, nullptr);

            Log::RtTrace("Destroying Vulkan swapchain image views.");
            for (const auto swapchainImageView : m_SwapchainImageViews) {
                vkDestroyImageView(device.GetDevice(), swapchainImageView, nullptr);
            }
        });

        m_Initialized = true;
    }

    Swapchain::~Swapchain() {
        Destroy();
    }

    VkResult Swapchain::AcquireNextImage(const Device& device, FrameData& frame) const {
        VK_CHECK(vkWaitForFences(device.GetDevice(), 1, &frame.RenderFence, true, 1000000000))

        frame.DeletionQueue.Flush();

        VK_CHECK(vkResetFences(device.GetDevice(), 1, &frame.RenderFence))

        return vkAcquireNextImageKHR(device.GetDevice(), m_Swapchain, 1000000000, frame.SwapchainSemaphore, nullptr,
                                     &
                                     frame.SwapchainImageIndex);
    }

    void Swapchain::Destroy() {
        if (m_Initialized) {
            m_DeletionQueue.Flush();
            m_Initialized = false;
        }
    }
}
