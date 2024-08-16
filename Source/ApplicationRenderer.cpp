// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Raytracer/ApplicationRenderer.hpp>

#include <Raytracer/Renderer/VulkanUtils/VulkanPipelineUtils.hpp>

namespace Raytracer {
    ApplicationRenderer::ApplicationRenderer(Renderer::VulkanRenderer* renderer) : m_Renderer(renderer) {
        InitializeDescriptors();
        InitializeRaytracePipeline();
    }

    ApplicationRenderer::~ApplicationRenderer() {
        vkDeviceWaitIdle(m_Renderer->GetDevice().GetDevice());

        m_DeletionQueue.Flush();
    }

    void ApplicationRenderer::Raytrace(const VkCommandBuffer commandBuffer) const {
        // Bind the gradient drawing compute pipeline.
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_RaytracingShader.Pipeline);

        // Bind the descriptor set containing the draw image for the compute pipeline.
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_RaytracingShader.Layout, 0, 1,
                                &m_DrawImageDescriptors, 0, nullptr);

        vkCmdPushConstants(commandBuffer, m_RaytracingShader.Layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                           sizeof(RaytracingPushConstants), &m_RaytracingShader.Data);

        // Execute the compute pipeline dispatch. The workgroup size is 16x16, so we need to divide by it.
        vkCmdDispatch(commandBuffer, static_cast<u32>(std::ceil(m_Renderer->DrawExtent.width / 16.0)),
                      static_cast<u32>(std::ceil(m_Renderer->DrawExtent.height / 16.0)), 1);
    }

    void ApplicationRenderer::InitializeDescriptors() {
        std::vector<Renderer::DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
        };

        m_DescriptorAllocator.Initialize(m_Renderer->GetDevice().GetDevice(), 10, sizes);

        {
            Renderer::DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            m_DrawImageDescriptorLayout = builder.Build(m_Renderer->GetDevice().GetDevice(),
                                                        VK_SHADER_STAGE_COMPUTE_BIT);
        }

        m_DrawImageDescriptors = m_DescriptorAllocator.Allocate(m_Renderer->GetDevice().GetDevice(),
                                                                m_DrawImageDescriptorLayout);

        Renderer::DescriptorWriter writer;
        writer.WriteImage(0, m_Renderer->DrawImage.ImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL,
                          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.UpdateSet(m_Renderer->GetDevice().GetDevice(), m_DrawImageDescriptors);

        m_DeletionQueue.PushFunction([&]() {
            m_DescriptorAllocator.DestroyPools(m_Renderer->GetDevice().GetDevice());

            vkDestroyDescriptorSetLayout(m_Renderer->GetDevice().GetDevice(), m_DrawImageDescriptorLayout, nullptr);
        });
    }

    void ApplicationRenderer::InitializeRaytracePipeline() {
        // Create pipeline layout
        VkPipelineLayoutCreateInfo computeLayout{};
        computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        computeLayout.pNext = nullptr;

        computeLayout.setLayoutCount = 1;
        computeLayout.pSetLayouts = &m_DrawImageDescriptorLayout;

        VkPushConstantRange pushConstant;
        pushConstant.offset = 0;
        pushConstant.size = sizeof(RaytracingPushConstants);
        pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        computeLayout.pushConstantRangeCount = 1;
        computeLayout.pPushConstantRanges = &pushConstant;

        VK_CHECK(
            vkCreatePipelineLayout(m_Renderer->GetDevice().GetDevice(), &computeLayout, nullptr, &m_RaytracingShader
                .Layout))

        // Create shader modules.
        VkShaderModule raytraceShaderModule;
        if (!Renderer::VulkanUtils::CreateShaderModule(m_Renderer->GetDevice().GetDevice(),
                                                       "Shaders/raytrace.comp.spv", &raytraceShaderModule)) {
            Log::RtFatal({0x02, 0x00}, "Failed to build the raytrace shader module.");
        }

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.pNext = nullptr;
        stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stageInfo.module = raytraceShaderModule;
        stageInfo.pName = "main";


        VkComputePipelineCreateInfo computePipelineCreateInfo{};
        computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computePipelineCreateInfo.pNext = nullptr;
        computePipelineCreateInfo.layout = m_RaytracingShader.Layout;
        computePipelineCreateInfo.stage = stageInfo;

        m_RaytracingShader.Data = {};

        VK_CHECK(
            vkCreateComputePipelines(m_Renderer->GetDevice().GetDevice(), VK_NULL_HANDLE, 1,
                &computePipelineCreateInfo, nullptr, &m_RaytracingShader.Pipeline))

        vkDestroyShaderModule(m_Renderer->GetDevice().GetDevice(), raytraceShaderModule, nullptr);

        m_DeletionQueue.PushFunction([this]() {
            vkDestroyPipelineLayout(m_Renderer->GetDevice().GetDevice(), m_RaytracingShader.Layout, nullptr);
            vkDestroyPipeline(m_Renderer->GetDevice().GetDevice(), m_RaytracingShader.Pipeline, nullptr);
        });
    }
}
