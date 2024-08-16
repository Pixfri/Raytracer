// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Raytracer/Renderer/VulkanWrapper/Device.hpp>

namespace Raytracer::Renderer::VulkanWrapper {
    Device::Device(const Instance& instance) {
        // Vulkan 1.3 features
        VkPhysicalDeviceVulkan13Features features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
        };
        features.dynamicRendering = VK_TRUE;
        features.synchronization2 = VK_TRUE;

        // Vulkan 1.2 features
        VkPhysicalDeviceVulkan12Features features12 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
        };
        features12.bufferDeviceAddress = VK_TRUE;
        features12.descriptorIndexing = VK_TRUE;

        Log::RtTrace("Selecting Vulkan physical device & creating Vulkan logical device...");
        vkb::PhysicalDeviceSelector selector{instance.GetVkbInstance()};
        vkb::PhysicalDevice physicalDevice = selector
                                             .set_minimum_version(1, 3)
                                             .set_required_features_13(features)
                                             .set_required_features_12(features12)
                                             .set_surface(instance.GetSurface())
                                             .select()
                                             .value();

        // Create the final Vulkan device.
        vkb::DeviceBuilder deviceBuilder{physicalDevice};

        vkb::Device vkbDevice = deviceBuilder.build().value();

        Log::RtTrace("Vulkan physical device selected & logical device created.");
        m_Device = vkbDevice.device;
        m_PhysicalDevice = physicalDevice.physical_device;

        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &physicalDeviceProperties);

        Log::RtTrace("Vulkan physical device properties:");
        Log::RtTrace("\t - Device name:           {0}", physicalDeviceProperties.deviceName);

        Log::RtTrace("\t - Device type:           {0}",
                     string_VkPhysicalDeviceType(physicalDeviceProperties.deviceType));

        Log::RtTrace("\t - Device API version:    {0}.{1}.{2}.{3}",
                     VK_API_VERSION_VARIANT(physicalDeviceProperties.apiVersion),
                     VK_API_VERSION_MAJOR(physicalDeviceProperties.apiVersion),
                     VK_API_VERSION_MINOR(physicalDeviceProperties.apiVersion),
                     VK_API_VERSION_PATCH(physicalDeviceProperties.apiVersion));

        // 4318 : NVIDIA
        if (physicalDeviceProperties.vendorID == 4318) {
            Log::RtTrace("\t - Driver version:        {0}.{1}.{2}.{3}",
                         (physicalDeviceProperties.driverVersion >> 22) & 0x3FF,
                         (physicalDeviceProperties.driverVersion >> 14) & 0x0FF,
                         (physicalDeviceProperties.driverVersion >> 6) & 0x0FF,
                         (physicalDeviceProperties.driverVersion) & 0x003F);
        }

#if defined(_WIN32) || defined(_WIN64)
        // 0x8086 : Intel, only differs on Windows.
        else if (physicalDeviceProperties.vendorID == 0x8086) {
            Log::RtTrace("\t - Driver version:          {0}.{1}",
                         (physicalDeviceProperties.driverVersion >> 14),
                         (physicalDeviceProperties.driverVersion) & 0x3FFF);
        }
#endif
        // For other vendors, use the Vulkan version convention.
        else {
            Log::RtTrace("\t - Driver version:          {0}.{0}.{0}",
                         VK_API_VERSION_MAJOR(physicalDeviceProperties.driverVersion),
                         VK_API_VERSION_MINOR(physicalDeviceProperties.driverVersion),
                         VK_API_VERSION_PATCH(physicalDeviceProperties.driverVersion));
        }

        m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        m_GraphicsQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
        m_PresentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
        m_PresentQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::present).value();

        m_DeletionQueue.PushFunction([this]() {
            Log::RtTrace("Destroying Vulkan device.");
            vkDestroyDevice(m_Device, nullptr);
        });

        m_Initialized = true;
    }

    Device::~Device() {
        if (m_Initialized) {
            m_DeletionQueue.Flush();
        }
    }
}
