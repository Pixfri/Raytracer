// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <Raytracer/Renderer/VulkanTypes.hpp>
#include <Raytracer/Renderer/VulkanWrapper/Instance.hpp>

namespace Raytracer::Renderer::VulkanWrapper {
    class Device {
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        vkb::Device m_VkbDevice;

        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        u32 m_GraphicsQueueFamilyIndex = 0;

        VkQueue m_PresentQueue = VK_NULL_HANDLE;
        u32 m_PresentQueueFamilyIndex = 0;

        DeletionQueue m_DeletionQueue;

        bool m_Initialized = false;

    public:
        explicit Device(const Instance& instance);
        ~Device();

        Device(const Device&) = delete;
        Device(Device&&) = delete;

        Device& operator=(const Device&) = delete;
        Device& operator=(Device&&) = delete;

        [[nodiscard]] inline VkPhysicalDevice GetPhysicalDevice() const;
        [[nodiscard]] inline VkDevice GetDevice() const;
        [[nodiscard]] inline vkb::Device GetVkbDevice() const;
        [[nodiscard]] inline VkQueue GetGraphicsQueue() const;
        [[nodiscard]] inline u32 GetGraphicsQueueFamilyIndex() const;
        [[nodiscard]] inline VkQueue GetPresentQueue() const;
        [[nodiscard]] inline u32 GetPresentQueueFamilyIndex() const;
    };

#include <Raytracer/Renderer/VulkanWrapper/Device.inl>
}
