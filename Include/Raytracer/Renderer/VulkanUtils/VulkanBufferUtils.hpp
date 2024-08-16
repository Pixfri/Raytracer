// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <Raytracer/Renderer/VulkanTypes.hpp>

namespace Raytracer::Renderer::VulkanUtils {
    AllocatedBuffer CreateBuffer(VmaAllocator allocator, usize allocSize, VkBufferUsageFlags usage,
                                 VmaMemoryUsage memoryUsage);
    void DestroyBuffer(VmaAllocator allocator, const AllocatedBuffer& buffer);
}
