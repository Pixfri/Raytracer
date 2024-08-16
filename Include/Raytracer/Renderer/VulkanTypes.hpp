/* Copyright (C) 2024 Jean "Pixfri" Letessier
 * This file is part of "Flashlight Engine"
 * For conditions of distribution and use, see copyright notice in LICENSE.
 *
 * File : VulkanTypes.hpp
 * Description : Header which includes different Vulkan headers used across the whole renderer, as well as a macro
 *               to check the return status of vulkan functions.
 */
#pragma once

#include <Raytracer/rtpch.hpp>
#include <Raytracer/Core/Logger.hpp>

#include <vulkan/vulkan.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <deque>
#include <functional>
#include <ranges>

namespace Raytracer {
    enum class DebugLevel : u8 {
        None = 0,

        Errors = 1,
        Warnings = 2,
        Verbose = 3,
        Debug = 4
    };

    namespace Renderer {
        struct DeletionQueue {
            std::deque<std::function<void()>> Deletors;

            void PushFunction(std::function<void()>&& function) {
                Deletors.push_back(std::move(function));
            }

            void Flush() {
                // Reverse iterate the deletion queue to execute all the functions.
                for (auto& deletor : std::ranges::reverse_view(Deletors)) {
                    deletor();
                }

                Deletors.clear();
            }
        };

        struct AllocatedImage {
            VkImage Image;
            VkImageView ImageView;
            VmaAllocation Allocation;
            VkExtent3D ImageExtent;
            VkFormat ImageFormat;
        };

        struct AllocatedBuffer {
            VkBuffer Buffer;
            VmaAllocation Allocation;
            VmaAllocationInfo Info;
        };

        struct SceneData {
            glm::mat4 View;
            glm::mat4 Projection;
            glm::mat4 ViewProjection;
            glm::vec4 AmbientColor;
            glm::vec4 SunlightDirection; // w for light intensity.
            glm::vec4 SunlightColor;
        };
    }
}

#define VK_CHECK(x)                                                            \
    do {                                                                       \
        VkResult err = x;                                                      \
        if (err) {                                                             \
            Raytracer::Log::RtError("Vulkan Error: {}", string_VkResult(err)); \
            abort();                                                           \
        }                                                                      \
    } while (0);
