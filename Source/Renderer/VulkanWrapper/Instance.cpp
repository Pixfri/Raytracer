// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Raytracer/Renderer/VulkanWrapper/Instance.hpp>

namespace Raytracer::Renderer::VulkanWrapper {
    namespace {
        VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            const VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData) {
            std::stringstream message;

            // Severity
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
                message << "[INFO] ";
            }

            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
                message << "[VERBOSE] ";
            }

            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                message << "[WARNING] ";
            }

            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
                message << "[ERROR] ";
            }

            // Type
            if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
                message << "[GENERAL] ";
            }

            if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
                message << "[VALIDATION] ";
            }

            if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
                message << "[PERFORMANCE] ";
            }

            message << '[' << pCallbackData->messageIdNumber << ' ';
            if (pCallbackData->pMessageIdName) {
                message << pCallbackData->pMessageIdName;
            }

            message << "]: " << pCallbackData->pMessage;

            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT ||
                messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
                Log::RtTrace(message.str());
            } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                Log::RtWarn(message.str());
            } else {
                Log::RtError(message.str());
            }

            return VK_FALSE;
        }
    }
    
    Instance::Instance(const Window& window, const DebugLevel& debugLevel) {
        Log::RtTrace("Creating Vulkan instance & debug messenger...");
        vkb::InstanceBuilder builder;

        u32 extensionCount = 0;
        glfwGetRequiredInstanceExtensions(&extensionCount);
        const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

        Log::RtTrace("Vulkan instance extensions required by the window:");
        for (u32 i = 0; i < extensionCount; i++) {
            Log::RtTrace("\t - {0}", extensions[i]);
        }

        builder.set_app_name(window.GetTitle().c_str())
               .set_app_version(0, 1, 0)
               .set_engine_name("Raytracer")
               .set_engine_version(0, 1, 0)
               .request_validation_layers(debugLevel > DebugLevel::None)
               .require_api_version(1, 3, 0);

        VkDebugUtilsMessageSeverityFlagsEXT messageSeverity = 0;

        if (debugLevel >= DebugLevel::Errors) {
            messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        }

        if (debugLevel >= DebugLevel::Warnings) {
            messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        }

        if (debugLevel >= DebugLevel::Verbose) {
            messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        }

        if (debugLevel >= DebugLevel::Debug) {
            messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        }

        constexpr VkDebugUtilsMessageTypeFlagsEXT messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        builder.add_debug_messenger_severity(messageSeverity)
               .add_debug_messenger_type(messageType)
               .set_debug_callback(DebugCallback);

        auto builderResult = builder.build();

        m_VkbInstance = builderResult.value();
        m_Instance = m_VkbInstance.instance;
        m_DebugMessenger = m_VkbInstance.debug_messenger;

        Log::RtTrace("Vulkan Instance & debug messenger created.");

        m_DeletionQueue.PushFunction([this]() {
            Log::RtTrace("Destroying Vulkan debug messenger.");
            vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger, nullptr);

            Log::RtTrace("Destroying Vulkan instance.");
            vkDestroyInstance(m_Instance, nullptr);
        });

        
        Log::RtTrace("Creating Vulkan window surface...");
        glfwCreateWindowSurface(m_Instance, window.GetNativeWindow(), nullptr, &m_Surface);
        Log::RtTrace("Vulkan window surface created.");

        m_DeletionQueue.PushFunction([this]() {
            Log::RtTrace("Destroying Vulkan surface.");
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
        });

        m_Initialized = true;
    }

    Instance::~Instance() {
        if (m_Initialized) {
            m_DeletionQueue.Flush();
            m_Initialized = false;
        }
    }
}
