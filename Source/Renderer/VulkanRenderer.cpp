// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Raytracer/Renderer/VulkanRenderer.hpp>

#include <Raytracer/Renderer/VulkanInitializers.hpp>
#include <Raytracer/Renderer/VulkanUtils/VulkanImageUtils.hpp>
#include <Raytracer/Renderer/VulkanUtils/VulkanPipelineUtils.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace Raytracer::Renderer {

    VulkanRenderer::VulkanRenderer(const Window& window, const DebugLevel& debugLevel) {
        InitializeVulkan(window, debugLevel);
        InitializeSwapchain(window);
        InitializeCommands();
        InitializeSynchronisationPrimitives();
        InitializeImGui(window);

        m_RendererInitialized = true;
    }

    VulkanRenderer::~VulkanRenderer() {
        if (m_RendererInitialized) {
            vkDeviceWaitIdle(m_Device->GetDevice());

            m_MainDeletionQueue.Flush();

            m_RendererInitialized = false;
        }
    }

    void VulkanRenderer::PlanDescriptorPoolsDeletion(DescriptorAllocatorGrowable& allocator) {
        m_MainDeletionQueue.PushFunction([this, &allocator]() {
            allocator.ClearPools(m_Device->GetDevice());
            allocator.DestroyPools(m_Device->GetDevice());
        });
    }

    void VulkanRenderer::PlanDeletion(std::function<void()>&& deletor) {
        m_MainDeletionQueue.PushFunction(std::move(deletor));
    }

    void VulkanRenderer::BeginUi() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();
    }

    VkCommandBuffer VulkanRenderer::BeginCommandBuffer(const Window& window) {
        ImGui::Render();

        const VkExtent2D swapchainExtent = m_Swapchain->GetSwapchainExtent();

        DrawExtent.width = static_cast<u32>(static_cast<f32>(std::min(
            swapchainExtent.width, DrawImage.ImageExtent.width)));
        DrawExtent.height = static_cast<u32>(static_cast<f32>(std::min(
            swapchainExtent.height, DrawImage.ImageExtent.height)));

        auto& frame = GetCurrentFrame();

        if (const VkResult lastVkError = m_Swapchain->AcquireNextImage(*m_Device, frame);
            lastVkError == VK_ERROR_OUT_OF_DATE_KHR || lastVkError == VK_SUBOPTIMAL_KHR || window.
            ShouldInvalidateSwapchain()) {
            m_SwapchainResizeRequired = true;
        }

        frame.DeletionQueue.Flush();
        frame.FrameDescriptors.ClearPools(m_Device->GetDevice());

        const VkCommandBuffer cmd = frame.MainCommandBuffer;

        VK_CHECK(vkResetCommandBuffer(cmd, 0))

        const VkCommandBufferBeginInfo cmdBeginInfo = VulkanInit::CommandBufferBeginInfo(
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo))

        VulkanUtils::TransitionImage(cmd, DrawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        return cmd;
    }

    void VulkanRenderer::EndCommandBuffer(Window& window) {
        const auto& frame = GetCurrentFrame();
        const auto cmd = frame.MainCommandBuffer;

        //transition the draw image and the swapchain image into their correct transfer layouts
        VulkanUtils::TransitionImage(cmd, DrawImage.Image, VK_IMAGE_LAYOUT_GENERAL,
                                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        VulkanUtils::TransitionImage(cmd, m_Swapchain->GetImageAtIndex(frame.SwapchainImageIndex),
                                     VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // execute a copy from the draw image into the swapchain
        VulkanUtils::CopyImageToImage(cmd, DrawImage.Image,
                                      m_Swapchain->GetImageAtIndex(frame.SwapchainImageIndex), DrawExtent,
                                      m_Swapchain->GetSwapchainExtent());

        DrawImGui(cmd, m_Swapchain->GetImageViewAtIndex(frame.SwapchainImageIndex));

        // set swapchain image layout to Present so we can show it on the screen
        VulkanUtils::TransitionImage(cmd, m_Swapchain->GetImageAtIndex(frame.SwapchainImageIndex),
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        VK_CHECK(vkEndCommandBuffer(cmd))

        const VkCommandBufferSubmitInfo cmdInfo = VulkanInit::CommandBufferSubmitInfo(cmd);

        const VkSemaphoreSubmitInfo waitInfo = VulkanInit::SemaphoreSubmitInfo(
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, frame.SwapchainSemaphore);
        const VkSemaphoreSubmitInfo signalInfo = VulkanInit::SemaphoreSubmitInfo(
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, frame.RenderSemaphore);

        const VkSubmitInfo2 submit = VulkanInit::SubmitInfo(&cmdInfo, &signalInfo, &waitInfo);

        VK_CHECK(vkQueueSubmit2(m_Device->GetGraphicsQueue(), 1, &submit, frame.RenderFence))

        const VkSwapchainKHR swapchain = m_Swapchain->GetSwapchain();

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.swapchainCount = 1;

        presentInfo.pWaitSemaphores = &frame.RenderSemaphore;
        presentInfo.waitSemaphoreCount = 1;

        presentInfo.pImageIndices = &frame.SwapchainImageIndex;

        if (const VkResult lastVkError = vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);
            lastVkError == VK_ERROR_OUT_OF_DATE_KHR || lastVkError == VK_SUBOPTIMAL_KHR || window.
            ShouldInvalidateSwapchain()) {
            m_SwapchainResizeRequired = true;
        }

        if (m_SwapchainResizeRequired) {
            RecreateSwapchain(window);
            m_SwapchainResizeRequired = false;
        }

        m_FrameNumber++;
    }

    void VulkanRenderer::ImmediateSubmit(const std::function<void(VkCommandBuffer commandBuffer)>& function) const {
        VK_CHECK(vkResetFences(m_Device->GetDevice(), 1, &m_ImmediateFence))
        VK_CHECK(vkResetCommandBuffer(m_ImmediateCommandBuffer, 0))

        const VkCommandBuffer commandBuffer = m_ImmediateCommandBuffer;

        const VkCommandBufferBeginInfo cmdBeginInfo = VulkanInit::CommandBufferBeginInfo(
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo))

        function(commandBuffer);

        VK_CHECK(vkEndCommandBuffer(commandBuffer))

        const VkCommandBufferSubmitInfo cmdInfo = VulkanInit::CommandBufferSubmitInfo(commandBuffer);
        const VkSubmitInfo2 submit = VulkanInit::SubmitInfo(&cmdInfo, nullptr, nullptr);

        // Submit command buffer to the queue and execute it.
        // m_RenderFence will now block until the graphics command finish executing.
        VK_CHECK(vkQueueSubmit2(m_Device->GetGraphicsQueue(), 1, &submit, m_ImmediateFence))

        VK_CHECK(vkWaitForFences(m_Device->GetDevice(), 1, &m_ImmediateFence, true, 9999999999))
    }

    void VulkanRenderer::InitializeVulkan(const Window& window, const DebugLevel& debugLevel) {
        m_Instance = std::make_unique<VulkanWrapper::Instance>(window, debugLevel);
        m_Device = std::make_unique<VulkanWrapper::Device>(*m_Instance);

        Log::RtTrace("Creating VMA allocator...");

        VmaVulkanFunctions functions{};
        functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = m_Device->GetPhysicalDevice();
        allocatorInfo.device = m_Device->GetDevice();
        allocatorInfo.instance = m_Instance->GetInstance();
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        allocatorInfo.pVulkanFunctions = &functions;
        vmaCreateAllocator(&allocatorInfo, &m_Allocator);

        Log::RtTrace("VMA allocator created.");

        m_MainDeletionQueue.PushFunction([&]() {
            Log::RtTrace("Destroying VMA allocator.");
            vmaDestroyAllocator(m_Allocator);
        });
    }

    void VulkanRenderer::InitializeSwapchain(const Window& window) {
        m_Swapchain = std::make_unique<VulkanWrapper::Swapchain>(window, *m_Instance, *m_Device);

        // Draw image size will match the window.
        const VkExtent3D drawImageExtent = {
            static_cast<u32>(window.GetWidth()), static_cast<u32>(window.GetHeight()), 1
        };

        DrawImage.ImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        DrawImage.ImageExtent = drawImageExtent;

        VkImageUsageFlags drawImageUsages{};
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const VkImageCreateInfo drawImageInfo = VulkanInit::ImageCreateInfo(
            DrawImage.ImageFormat, drawImageUsages, drawImageExtent);

        // For the draw image, we want to allocate it from GPU local memory.
        VmaAllocationCreateInfo drawImageAllocInfo{};
        drawImageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        drawImageAllocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        // Allocate and create the image.
        vmaCreateImage(m_Allocator, &drawImageInfo, &drawImageAllocInfo, &DrawImage.Image,
                       &DrawImage.Allocation, nullptr);

        // Build an image view for the draw image to use for rendering.
        const VkImageViewCreateInfo drawImageViewInfo = VulkanInit::ImageViewCreateInfo(
            DrawImage.ImageFormat, DrawImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);

        VK_CHECK(vkCreateImageView(m_Device->GetDevice(), &drawImageViewInfo, nullptr, &DrawImage.ImageView))

        // Add to deletion queue.
        m_MainDeletionQueue.PushFunction([this]() {
            vkDestroyImageView(m_Device->GetDevice(), DrawImage.ImageView, nullptr);
            vmaDestroyImage(m_Allocator, DrawImage.Image, DrawImage.Allocation);
        });
    }

    void VulkanRenderer::InitializeCommands() {
        InitializeFramesCommandBuffers();
        InitializeImmediateCommandBuffer();
    }

    void VulkanRenderer::InitializeFramesCommandBuffers() {
        const VkCommandPoolCreateInfo commandPoolInfo = VulkanInit::CommandPoolCreateInfo(
            m_Device->GetGraphicsQueueFamilyIndex(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        Log::RtTrace("Creating frames Vulkan command pool & command buffer...");
        for (u32 i = 0; i < g_FrameOverlap; i++) {
            VK_CHECK(
                vkCreateCommandPool(m_Device->GetDevice(), &commandPoolInfo, nullptr, &m_Frames[i].CommandPool))

            Log::RtTrace("Created Vulkan command pool for frame #{0}", i);

            VkCommandBufferAllocateInfo cmdAllocInfo = VulkanInit::CommandBufferAllocateInfo(
                m_Frames[i].CommandPool);

            VK_CHECK(vkAllocateCommandBuffers(m_Device->GetDevice(), &cmdAllocInfo, &m_Frames[i].MainCommandBuffer))
            Log::RtTrace("Created Vulkan command command for frame #{0}", i);
        }

        m_MainDeletionQueue.PushFunction([this]() {
            for (u32 i = 0; i < g_FrameOverlap; i++) {
                Log::RtTrace("Destroying Vulkan command pool for frame #{0}.", i);
                vkDestroyCommandPool(m_Device->GetDevice(), m_Frames[i].CommandPool, nullptr);
            }
        });
    }

    void VulkanRenderer::InitializeImmediateCommandBuffer() {
        // Command pool/buffer for immediate commands like copy commands.
        const VkCommandPoolCreateInfo commandPoolInfo = VulkanInit::CommandPoolCreateInfo(
            m_Device->GetGraphicsQueueFamilyIndex(),
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

        Log::RtTrace("Creating Vulkan command pool for immediate command buffers...");
        VK_CHECK(vkCreateCommandPool(m_Device->GetDevice(), &commandPoolInfo, nullptr, &m_ImmediateCommandPool))
        Log::RtTrace("Vulkan command pool for immediate command buffers created.");

        // Allocate command buffers for immediate submits.
        const VkCommandBufferAllocateInfo cmdAllocInfo = VulkanInit::CommandBufferAllocateInfo(
            m_ImmediateCommandPool);

        Log::RtTrace("Creating Vulkan immediate command buffer...");
        VK_CHECK(vkAllocateCommandBuffers(m_Device->GetDevice(), &cmdAllocInfo, &m_ImmediateCommandBuffer))
        Log::RtTrace("Vulkan immediate command buffer created.");

        m_MainDeletionQueue.PushFunction([this]() {
            vkDestroyCommandPool(m_Device->GetDevice(), m_ImmediateCommandPool, nullptr);
        });
    }

    void VulkanRenderer::InitializeSynchronisationPrimitives() {
        const VkFenceCreateInfo fenceCreateInfo = VulkanInit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        const VkSemaphoreCreateInfo semaphoreCreateInfo = VulkanInit::SemaphoreCreateInfo();

        Log::RtTrace("Creating Vulkan synchronisation primitives...");
        for (u32 i = 0; i < g_FrameOverlap; i++) {
            VK_CHECK(
                vkCreateFence(m_Device->GetDevice(), &fenceCreateInfo, nullptr, &m_Frames[i].RenderFence))
            Log::RtTrace("Vulkan render fence created for frame #{0}", i);

            VK_CHECK(
                vkCreateSemaphore(m_Device->GetDevice(), &semaphoreCreateInfo, nullptr, &m_Frames[i].
                    SwapchainSemaphore))
            Log::RtTrace("Vulkan swapchain semaphore created for frame #{0}", i);
            VK_CHECK(
                vkCreateSemaphore(m_Device->GetDevice(), &semaphoreCreateInfo, nullptr, &m_Frames[i].
                    RenderSemaphore))
            Log::RtTrace("Vulkan render semaphore created for frame #{0}", i);
        }

        m_MainDeletionQueue.PushFunction([this]() {
            for (u32 i = 0; i < g_FrameOverlap; i++) {
                Log::RtTrace("Destroying Vulkan synchronisation objects for frame #{0}.", i);
                vkDestroyFence(m_Device->GetDevice(), m_Frames[i].RenderFence, nullptr);
                vkDestroySemaphore(m_Device->GetDevice(), m_Frames[i].RenderSemaphore, nullptr);
                vkDestroySemaphore(m_Device->GetDevice(), m_Frames[i].SwapchainSemaphore, nullptr);

                m_Frames[i].DeletionQueue.Flush();
            }
        });

        Log::RtTrace("Creating Vulkan fence for immediate commands...");
        VK_CHECK(vkCreateFence(m_Device->GetDevice(), &fenceCreateInfo, nullptr, &m_ImmediateFence))
        Log::RtTrace("Vulkan fence for immediate commands created.");

        m_MainDeletionQueue.PushFunction([this]() {
            vkDestroyFence(m_Device->GetDevice(), m_ImmediateFence, nullptr);
        });
    }

    void VulkanRenderer::InitializeImGui(const Window& window) {
        // 1: Create descriptor pool for ImGui
        //    The pool is very oversize, but it's copied from ImGui demo
        //    itself.
        const VkDescriptorPoolSize poolSizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000;
        poolInfo.poolSizeCount = static_cast<u32>(std::size(poolSizes));
        poolInfo.pPoolSizes = poolSizes;

        VkDescriptorPool imguiPool;
        VK_CHECK(vkCreateDescriptorPool(m_Device->GetDevice(), &poolInfo, nullptr, &imguiPool))

        // 2: Initialize ImGui library.

        ImGui::CreateContext();

        ImGui_ImplGlfw_InitForVulkan(window.GetNativeWindow(), true);

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = m_Instance->GetInstance();
        initInfo.PhysicalDevice = m_Device->GetPhysicalDevice();
        initInfo.Device = m_Device->GetDevice();
        initInfo.QueueFamily = m_Device->GetGraphicsQueueFamilyIndex();
        initInfo.Queue = m_Device->GetGraphicsQueue();
        initInfo.DescriptorPool = imguiPool;
        initInfo.MinImageCount = 3;
        initInfo.ImageCount = 3;
        initInfo.UseDynamicRendering = true;

        const VkFormat swapchainImageFormat = m_Swapchain->GetSwapchainImageFormat();
        // Dynamic rendering parameters for ImGui to use.
        initInfo.PipelineRenderingCreateInfo = {};
        initInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainImageFormat;

        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        VkInstance instance = m_Instance->GetInstance();

        ImGui_ImplVulkan_LoadFunctions([](const char* functionName, void* vulkanInstance) {
            return vkGetInstanceProcAddr(*(static_cast<VkInstance*>(vulkanInstance)), functionName);
        }, &instance);

        ImGui_ImplVulkan_Init(&initInfo);

        ImGui_ImplVulkan_CreateFontsTexture();

        m_MainDeletionQueue.PushFunction([this, imguiPool]() {
            ImGui_ImplVulkan_Shutdown();
            vkDestroyDescriptorPool(m_Device->GetDevice(), imguiPool, nullptr);
        });
    }

    void VulkanRenderer::DrawImGui(const VkCommandBuffer commandBuffer, const VkImageView targetImageView) const {
        const VkRenderingAttachmentInfo colorAttachmentInfo = VulkanInit::AttachmentInfo(
            targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        const VkRenderingInfo renderInfo = VulkanInit::RenderingInfo(m_Swapchain->GetSwapchainExtent(),
                                                                     &colorAttachmentInfo, nullptr);

        vkCmdBeginRendering(commandBuffer, &renderInfo);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        vkCmdEndRendering(commandBuffer);
    }

    void VulkanRenderer::RecreateSwapchain(Window& window) {
        vkDeviceWaitIdle(m_Device->GetDevice());

        m_Swapchain->Destroy();

        m_Swapchain = std::make_unique<VulkanWrapper::Swapchain>(window, *m_Instance, *m_Device);

        window.SwapchainInvalidated();
    }
}
