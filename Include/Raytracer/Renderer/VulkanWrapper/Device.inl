// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

inline VkPhysicalDevice Device::GetPhysicalDevice() const {
    return m_PhysicalDevice;
}

inline VkDevice Device::GetDevice() const {
    return m_Device;
}

inline vkb::Device Device::GetVkbDevice() const {
    return m_VkbDevice;
}

inline VkQueue Device::GetGraphicsQueue() const {
    return m_GraphicsQueue;
}

inline u32 Device::GetGraphicsQueueFamilyIndex() const {
    return m_GraphicsQueueFamilyIndex;
}

inline VkQueue Device::GetPresentQueue() const {
    return m_PresentQueue;
}

inline u32 Device::GetPresentQueueFamilyIndex() const {
    return m_PresentQueueFamilyIndex;
}
