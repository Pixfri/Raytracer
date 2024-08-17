// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Raytracer/RaytracerApp/RaytracingRenderer.hpp>

#include <Raytracer/Renderer/VulkanUtils/VulkanBufferUtils.hpp>
#include <Raytracer/Renderer/VulkanUtils/VulkanPipelineUtils.hpp>

namespace Raytracer {
    RaytracingRenderer::RaytracingRenderer(Renderer::VulkanRenderer* renderer) : m_Renderer(renderer) {
    }

    RaytracingRenderer::~RaytracingRenderer() {
        vkDeviceWaitIdle(m_Renderer->GetDevice().GetDevice());

        m_DeletionQueue.Flush();
    }

    void RaytracingRenderer::CreateBottomLevelAccelerationStructure() {
        struct Vertex {
            f32 Position[3];
        };
        std::vector<Vertex> vertices = {
            {{1.0f, 1.0f, 0.0f}},
            {{-1.0f, 1.0f, 0.0f}},
            {{0.0f, -1.0f, 0.0f}},
        };
        std::vector<u32> indices = {0, 1, 2};

        const auto vertexBufferSize = vertices.size() * sizeof(Vertex);
        const auto indexBufferSize = indices.size() * sizeof(u32);

        constexpr VkBufferUsageFlags bufferUsageFlags =
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        // Create buffers for the bottom level geometry.
        VertexBuffer = Renderer::VulkanUtils::CreateBuffer(m_Renderer->GetAllocator(), vertexBufferSize,
                                                           bufferUsageFlags |
                                                           VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                           VMA_MEMORY_USAGE_GPU_ONLY);

        IndexBuffer = Renderer::VulkanUtils::CreateBuffer(m_Renderer->GetAllocator(), indexBufferSize,
                                                          bufferUsageFlags |
                                                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                          VMA_MEMORY_USAGE_GPU_ONLY);

        m_DeletionQueue.PushFunction([this]() {
            Renderer::VulkanUtils::DestroyBuffer(m_Renderer->GetAllocator(), VertexBuffer);
            Renderer::VulkanUtils::DestroyBuffer(m_Renderer->GetAllocator(), IndexBuffer);
        });

        // Staging buffer
        const Renderer::AllocatedBuffer staging = Renderer::VulkanUtils::CreateBuffer(
            m_Renderer->GetAllocator(), vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY);

        VmaAllocationInfo stagingBufferAllocationInfo;
        vmaGetAllocationInfo(m_Renderer->GetAllocator(), staging.Allocation, &stagingBufferAllocationInfo);

        void* data = stagingBufferAllocationInfo.pMappedData;

        memcpy(data, vertices.data(), vertexBufferSize);
        memcpy(static_cast<char*>(data) + vertexBufferSize, indices.data(), indexBufferSize);

        m_Renderer->ImmediateSubmit([&](const VkCommandBuffer commandBuffer) {
            VkBufferCopy vertexCopy;
            vertexCopy.srcOffset = 0;
            vertexCopy.dstOffset = 0;
            vertexCopy.size = vertexBufferSize;

            vkCmdCopyBuffer(commandBuffer, staging.Buffer, VertexBuffer.Buffer, 1, &vertexCopy);

            VkBufferCopy indexCopy;
            vertexCopy.srcOffset = vertexBufferSize;
            vertexCopy.dstOffset = 0;
            vertexCopy.size = indexBufferSize;

            vkCmdCopyBuffer(commandBuffer, staging.Buffer, IndexBuffer.Buffer, 1, &indexCopy);
        });

        Renderer::VulkanUtils::DestroyBuffer(m_Renderer->GetAllocator(), staging);

        constexpr VkTransformMatrixKHR transformMatrix = {
            {
                {1.0f, 0.0f, 0.0f, 0.0f},
                {0.0f, 1.0f, 0.0f, 0.0f},
                {0.0f, 0.0f, 1.0f, 0.0f}
            },
        };

        Renderer::AllocatedBuffer transformMatrixBuffer = Renderer::VulkanUtils::CreateBuffer(
            m_Renderer->GetAllocator(), sizeof(transformMatrix), bufferUsageFlags, VMA_MEMORY_USAGE_CPU_TO_GPU);

        m_DeletionQueue.PushFunction([this, transformMatrixBuffer]() {
            Renderer::VulkanUtils::DestroyBuffer(m_Renderer->GetAllocator(), transformMatrixBuffer);
        });

        VmaAllocationInfo transformMatrixBufferAllocationInfo;
        vmaGetAllocationInfo(m_Renderer->GetAllocator(), transformMatrixBuffer.Allocation,
                             &transformMatrixBufferAllocationInfo);

        data = transformMatrixBufferAllocationInfo.pMappedData;

        memcpy(data, &transformMatrix, sizeof(transformMatrix));

        VkDeviceOrHostAddressConstKHR vertexDataDeviceAddress{};
        VkDeviceOrHostAddressConstKHR indexDataDeviceAddress{};
        VkDeviceOrHostAddressConstKHR transformMatrixDeviceAddress{};

        vertexDataDeviceAddress.deviceAddress = GetBufferDeviceAddress(VertexBuffer.Buffer);
        indexDataDeviceAddress.deviceAddress = GetBufferDeviceAddress(IndexBuffer.Buffer);
        transformMatrixDeviceAddress.deviceAddress = GetBufferDeviceAddress(transformMatrixBuffer.Buffer);

        // The bottom level acceleration structure contains one set of triangles as the input geometry.
        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
        accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;
        accelerationStructureGeometry.geometry.triangles.sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        accelerationStructureGeometry.geometry.triangles.vertexData = vertexDataDeviceAddress;
        accelerationStructureGeometry.geometry.triangles.maxVertex = 3;
        accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
        accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        accelerationStructureGeometry.geometry.triangles.indexData = indexDataDeviceAddress;
        accelerationStructureGeometry.geometry.triangles.transformData = transformMatrixDeviceAddress;

        // Get the size requirements for buffers involved in the acceleration structure build process.
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

        constexpr u32 primitiveCount = 1;

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(m_Renderer->GetDevice().GetDevice(),
                                                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                &accelerationStructureBuildGeometryInfo, &primitiveCount,
                                                &accelerationStructureBuildSizesInfo);

        // Create a buffer to hold the acceleration structure.
        BottomLevelAccelerationStructure.Buffer = Renderer::VulkanUtils::CreateBuffer(
            m_Renderer->GetAllocator(), accelerationStructureBuildSizesInfo.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

        // Create the acceleration structure.
        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = BottomLevelAccelerationStructure.Buffer.Buffer;
        accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        VK_CHECK(
            vkCreateAccelerationStructureKHR(m_Renderer->GetDevice().GetDevice(), &accelerationStructureCreateInfo,
                nullptr, &BottomLevelAccelerationStructure.Handle))

        m_DeletionQueue.PushFunction([this]() {
            DeleteAccelerationStructure(BottomLevelAccelerationStructure);
        });

        // The actual build process starts here.

        // Create a scratch buffer as a temporary storage for the acceleration structure build.
        ScratchBuffer scratchBuffer = CreateScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
        accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationBuildGeometryInfo.dstAccelerationStructure = BottomLevelAccelerationStructure.Handle;
        accelerationBuildGeometryInfo.geometryCount = 1;
        accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
        accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.DeviceAddress;

        VkAccelerationStructureBuildRangeInfoKHR accelerationBuildRangeInfo{};
        accelerationBuildRangeInfo.primitiveCount = 1;
        accelerationBuildRangeInfo.primitiveOffset = 0;
        accelerationBuildRangeInfo.firstVertex = 0;
        accelerationBuildRangeInfo.transformOffset = 0;
        std::vector accelerationStructureBuildRangeInfos = {
            &accelerationBuildRangeInfo
        };

        // Build the acceleration structure on the device via one-time command buffer submission.
        // Some implementations may support acceleration structure building on the host
        // (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands),
        // but we prefer device builds.
        m_Renderer->ImmediateSubmit([&](const VkCommandBuffer commandBuffer) {
            vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationBuildGeometryInfo,
                                                accelerationStructureBuildRangeInfos.data());
        });

        DeleteScratchBuffer(scratchBuffer);

        // Get the bottom acceleration structure's handle, which will be used during the top level acceleration
        // build.
        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
        accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationDeviceAddressInfo.accelerationStructure = BottomLevelAccelerationStructure.Handle;
        BottomLevelAccelerationStructure.DeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(
            m_Renderer->GetDevice().GetDevice(), &accelerationDeviceAddressInfo);
    }

    /*
     * Create the top level acceleration structure containing geometry instances of the bottom level acceleration
     * structure(s).
     */
    void RaytracingRenderer::CreateTopLevelAccelerationStructure() {
        constexpr VkTransformMatrixKHR transformMatrix = {
            {
                {1.0f, 0.0f, 0.0f, 0.0f},
                {0.0f, 1.0f, 0.0f, 0.0f},
                {0.0f, 0.0f, 1.0f, 0.0f}
            },
        };

        VkAccelerationStructureInstanceKHR accelerationStructureInstance{};
        accelerationStructureInstance.transform = transformMatrix;
        accelerationStructureInstance.instanceCustomIndex = 0;
        accelerationStructureInstance.mask = 0xFF;
        accelerationStructureInstance.instanceShaderBindingTableRecordOffset = 0;
        accelerationStructureInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        accelerationStructureInstance.accelerationStructureReference = BottomLevelAccelerationStructure.
            DeviceAddress;

        Renderer::AllocatedBuffer instancesBuffer = Renderer::VulkanUtils::CreateBuffer(
            m_Renderer->GetAllocator(), sizeof(VkAccelerationStructureInstanceKHR),
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        VmaAllocationInfo instancesBufferAllocationInfo;
        vmaGetAllocationInfo(m_Renderer->GetAllocator(), instancesBuffer.Allocation,
                             &instancesBufferAllocationInfo);

        void* data = instancesBufferAllocationInfo.pMappedData;

        memcpy(data, &accelerationStructureInstance, sizeof(VkAccelerationStructureInstanceKHR));

        VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
        instanceDataDeviceAddress.deviceAddress = GetBufferDeviceAddress(instancesBuffer.Buffer);

        // The top level acceleration structure contains (bottom level) instance as the input geometry.
        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
        accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructureGeometry.geometry.instances.sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
        accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

        // Get the size requirements for buffers involved in the acceleration structure build process.
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

        constexpr u32 primitiveCount = 1;

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(m_Renderer->GetDevice().GetDevice(),
                                                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                &accelerationStructureBuildGeometryInfo, &primitiveCount,
                                                &accelerationStructureBuildSizesInfo);

        // Create a buffer to hold the acceleration structure.
        TopLevelAccelerationStructure.Buffer = Renderer::VulkanUtils::CreateBuffer(
            m_Renderer->GetAllocator(), accelerationStructureBuildSizesInfo.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

        // Create the acceleration structure.
        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = TopLevelAccelerationStructure.Buffer.Buffer;
        accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        VK_CHECK(
            vkCreateAccelerationStructureKHR(m_Renderer->GetDevice().GetDevice(), &accelerationStructureCreateInfo,
                nullptr, &TopLevelAccelerationStructure.Handle))
        
        m_DeletionQueue.PushFunction([this]() {
            DeleteAccelerationStructure(TopLevelAccelerationStructure);
        });

        // The actual build process starts here

        // Create a scratch buffer as a temporary storage for the acceleration structure build.
        ScratchBuffer scratchBuffer = CreateScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationStructureBuildGeometryInfo.dstAccelerationStructure = TopLevelAccelerationStructure.Handle;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
        accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.DeviceAddress;

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo;
        accelerationStructureBuildRangeInfo.primitiveCount = 1;
        accelerationStructureBuildRangeInfo.primitiveOffset = 0;
        accelerationStructureBuildRangeInfo.firstVertex = 0;
        accelerationStructureBuildRangeInfo.transformOffset = 0;
        std::vector accelerationStructureBuildRangeInfos = {
            &accelerationStructureBuildRangeInfo
        };

        // Build the acceleration structure on the device via one-time command buffer submission.
        // Some implementations may support acceleration structure building on the host
        // (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands),
        // but we prefer device builds.
        m_Renderer->ImmediateSubmit([&](const VkCommandBuffer commandBuffer) {
            vkCmdBuildAccelerationStructuresKHR(
                commandBuffer,
                1,
                &accelerationBuildGeometryInfo,
                accelerationStructureBuildRangeInfos.data());
        });

        DeleteScratchBuffer(scratchBuffer);

        // Get the acceleration structure's handle, which will be used to set up its descriptor.
        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
        accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationDeviceAddressInfo.accelerationStructure = TopLevelAccelerationStructure.Handle;
        TopLevelAccelerationStructure.DeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(
            m_Renderer->GetDevice().GetDevice(), &accelerationDeviceAddressInfo);
    }

    void RaytracingRenderer::DeleteAccelerationStructure(const AccelerationStructure& accelerationStructure) const {
        if (accelerationStructure.Buffer.Buffer != VK_NULL_HANDLE) {
            Renderer::VulkanUtils::DestroyBuffer(m_Renderer->GetAllocator(), accelerationStructure.Buffer);
        }

        if (accelerationStructure.Handle != VK_NULL_HANDLE) {
            vkDestroyAccelerationStructureKHR(m_Renderer->GetDevice().GetDevice(), accelerationStructure.Handle,
                                              nullptr);
        }
    }

    ScratchBuffer RaytracingRenderer::CreateScratchBuffer(const VkDeviceSize size) const {
        ScratchBuffer scratchBuffer;
        scratchBuffer.Buffer = Renderer::VulkanUtils::CreateBuffer(m_Renderer->GetAllocator(), size,
                                                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                                   VMA_MEMORY_USAGE_GPU_ONLY);
        scratchBuffer.DeviceAddress = GetBufferDeviceAddress(scratchBuffer.Buffer.Buffer);

        return scratchBuffer;
    }

    void RaytracingRenderer::DeleteScratchBuffer(const ScratchBuffer& scratchBuffer) const {
        Renderer::VulkanUtils::DestroyBuffer(m_Renderer->GetAllocator(), scratchBuffer.Buffer);
    }

    VkDeviceAddress RaytracingRenderer::GetBufferDeviceAddress(const VkBuffer buffer) const {
        const VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext = nullptr,
            .buffer = buffer
        };

        return vkGetBufferDeviceAddress(m_Renderer->GetDevice().GetDevice(), &bufferDeviceAddressInfo);
    }
}
