#include <app/ModelViewerRenderer.hpp>
#include <app/Renderer.hpp>
#include <iostream>
#include <stdexcept>

namespace sauce {

    ModelViewerRenderer::ModelViewerRenderer(const RendererCreateInfo& createInfo) {
        queueIndex = createInfo.logicalDevice.getQueueIndex();
        pQueue = std::make_unique<vk::raii::Queue>(*createInfo.logicalDevice, queueIndex, 0);

        pSwapChain =
            std::make_unique<sauce::SwapChain>(createInfo.physicalDevice, createInfo.logicalDevice,
                                               createInfo.renderSurface, createInfo.window);

        createDescriptorSetLayout(createInfo.logicalDevice);
        sauce::GraphicsPipelineConfig mainPipelineConfig{
            .physicalDevice = createInfo.physicalDevice,
            .logicalDevice = createInfo.logicalDevice,
            .descriptorSetLayouts = {*descriptorSetLayout},
            .colorFormat = pSwapChain->getSurfaceFormat().format,
        };
        pPipeline = std::make_unique<sauce::GraphicsPipeline>(
            mainPipelineConfig, "shaders/shader_3d.vert.spv", "shaders/shader_3d.frag.spv");

        vk::CommandPoolCreateInfo commandPoolCreateInfo{
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueIndex,
        };

        commandPool = vk::raii::CommandPool{*createInfo.logicalDevice, commandPoolCreateInfo};

        vk::CommandBufferAllocateInfo allocInfo{
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
        };

        commandBuffers = vk::raii::CommandBuffers(*createInfo.logicalDevice, allocInfo);

        createUniformBuffers(createInfo.physicalDevice, createInfo.logicalDevice);
        createDescriptorSets(createInfo.logicalDevice);

        for (size_t i = 0; i < pSwapChain->getImages().size(); ++i) {
            renderFinishedSemaphores.emplace_back(*createInfo.logicalDevice,
                                                  vk::SemaphoreCreateInfo{});
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            presentCompleteSemaphores.emplace_back(*createInfo.logicalDevice,
                                                   vk::SemaphoreCreateInfo{});
            inFlightFences.emplace_back(
                *createInfo.logicalDevice,
                vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
        }
    }

    void ModelViewerRenderer::loadModel(const std::string& modelPath,
                                        const sauce::PhysicalDevice& physicalDevice,
                                        const sauce::LogicalDevice& logicalDevice) {
        std::cout << "Loading model: " << modelPath << std::endl;

        sauce::modeling::GLTFLoader loader;
        pModel = loader.loadModel(modelPath);

        if (!pModel) {
            throw std::runtime_error("Failed to load model: " + modelPath);
        }

        const auto& meshes = pModel->getAllMeshes();
        std::cout << "Model loaded with " << meshes.size() << " mesh(es)" << std::endl;

        if (meshes.empty()) {
            throw std::runtime_error("Model has no meshes: " + modelPath);
        }

        for (const auto& mesh : meshes) {
            uploadMeshToGPU(mesh, physicalDevice, logicalDevice);
        }

        std::cout << "All meshes uploaded to GPU" << std::endl;
    }

    void ModelViewerRenderer::uploadMeshToGPU(const std::shared_ptr<sauce::modeling::Mesh>& mesh,
                                              const sauce::PhysicalDevice& physicalDevice,
                                              const sauce::LogicalDevice& logicalDevice) {
        MeshGPUData gpuData;

        const auto& vertices = mesh->getVertices();
        const auto& indices = mesh->getIndices();

        // Upload vertex buffer
        vk::DeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();

        vk::raii::Buffer stagingVertexBuffer = nullptr;
        vk::raii::DeviceMemory stagingVertexBufferMemory = nullptr;
        sauce::BufferUtils::createBuffer(
            physicalDevice, logicalDevice, vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            stagingVertexBuffer, stagingVertexBufferMemory);

        void* dataStaging = stagingVertexBufferMemory.mapMemory(0, vertexBufferSize);
        memcpy(dataStaging, vertices.data(), vertexBufferSize);
        stagingVertexBufferMemory.unmapMemory();

        sauce::BufferUtils::createBuffer(physicalDevice, logicalDevice, vertexBufferSize,
                                         vk::BufferUsageFlagBits::eVertexBuffer |
                                             vk::BufferUsageFlagBits::eTransferDst,
                                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                                         gpuData.vertexBuffer, gpuData.vertexBufferMemory);

        sauce::BufferUtils::copyBuffer(logicalDevice, commandPool, *pQueue, stagingVertexBuffer,
                                       gpuData.vertexBuffer, vertexBufferSize);

        // Upload index buffer
        vk::DeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();

        vk::raii::Buffer stagingIndexBuffer = nullptr;
        vk::raii::DeviceMemory stagingIndexBufferMemory = nullptr;
        sauce::BufferUtils::createBuffer(
            physicalDevice, logicalDevice, indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            stagingIndexBuffer, stagingIndexBufferMemory);

        void* indexDataStaging = stagingIndexBufferMemory.mapMemory(0, indexBufferSize);
        memcpy(indexDataStaging, indices.data(), indexBufferSize);
        stagingIndexBufferMemory.unmapMemory();

        sauce::BufferUtils::createBuffer(physicalDevice, logicalDevice, indexBufferSize,
                                         vk::BufferUsageFlagBits::eIndexBuffer |
                                             vk::BufferUsageFlagBits::eTransferDst,
                                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                                         gpuData.indexBuffer, gpuData.indexBufferMemory);

        sauce::BufferUtils::copyBuffer(logicalDevice, commandPool, *pQueue, stagingIndexBuffer,
                                       gpuData.indexBuffer, indexBufferSize);

        gpuData.indexCount = static_cast<uint32_t>(indices.size());

        meshBuffers.push_back(std::move(gpuData));
    }

    void ModelViewerRenderer::createDescriptorSetLayout(const sauce::LogicalDevice& logicalDevice) {
        vk::DescriptorSetLayoutBinding uboLayoutBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
        };
        vk::DescriptorSetLayoutCreateInfo dsLayoutInfo{
            .bindingCount = 1,
            .pBindings = &uboLayoutBinding,
        };

        descriptorSetLayout = vk::raii::DescriptorSetLayout{*logicalDevice, dsLayoutInfo};
    }

    void ModelViewerRenderer::createDescriptorSets(const sauce::LogicalDevice& logicalDevice) {
        vk::DescriptorPoolSize poolSize{
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = MAX_FRAMES_IN_FLIGHT,
        };

        vk::DescriptorPoolCreateInfo poolCreateInfo{
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = MAX_FRAMES_IN_FLIGHT,
            .poolSizeCount = 1,
            .pPoolSizes = &poolSize,
        };

        descriptorPool = vk::raii::DescriptorPool{*logicalDevice, poolCreateInfo};

        std::vector<vk::DescriptorSetLayout> layouts{MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout};
        vk::DescriptorSetAllocateInfo dsAllocInfo{.descriptorPool = descriptorPool,
                                                  .descriptorSetCount =
                                                      static_cast<uint32_t>(layouts.size()),
                                                  .pSetLayouts = layouts.data()};

        descriptorSets = logicalDevice->allocateDescriptorSets(dsAllocInfo);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vk::DescriptorBufferInfo bufferInfo{
                .buffer = uniformBuffers[i],
                .offset = 0,
                .range = sizeof(UniformBufferObject),
            };

            vk::WriteDescriptorSet descriptorWrite{
                .dstSet = descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &bufferInfo,
            };

            logicalDevice->updateDescriptorSets(descriptorWrite, {});
        }
    }

    void ModelViewerRenderer::createUniformBuffers(const sauce::PhysicalDevice& physicalDevice,
                                                   const sauce::LogicalDevice& logicalDevice) {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vk::DeviceSize size = sizeof(UniformBufferObject);
            vk::raii::Buffer buf = nullptr;
            vk::raii::DeviceMemory mem = nullptr;
            sauce::BufferUtils::createBuffer(physicalDevice, logicalDevice, size,
                                             vk::BufferUsageFlagBits::eUniformBuffer,
                                             vk::MemoryPropertyFlagBits::eHostCoherent |
                                                 vk::MemoryPropertyFlagBits::eHostVisible,
                                             buf, mem);

            uniformBuffers.emplace_back(std::move(buf));
            uniformBuffersMemory.emplace_back(std::move(mem));
            uniformBuffersMapped.emplace_back(uniformBuffersMemory[i].mapMemory(0, size));
        }
    }

    void ModelViewerRenderer::transitionImageLayout(uint32_t imageIndex, vk::ImageLayout oldLayout,
                                                    vk::ImageLayout newLayout,
                                                    vk::AccessFlags2 srcAccessMask,
                                                    vk::AccessFlags2 dstAccessMask,
                                                    vk::PipelineStageFlags2 srcStageMask,
                                                    vk::PipelineStageFlags2 dstStageMask) {
        vk::ImageMemoryBarrier2 barrier{
            .srcStageMask = srcStageMask,
            .srcAccessMask = srcAccessMask,
            .dstStageMask = dstStageMask,
            .dstAccessMask = dstAccessMask,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = pSwapChain->getImages()[imageIndex],
            .subresourceRange =
                {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };

        vk::DependencyInfo dependencyInfo{
            .dependencyFlags = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier,
        };

        commandBuffers[frameIndex].pipelineBarrier2(dependencyInfo);
    }

    void ModelViewerRenderer::recordCommandBuffer(uint32_t imageIndex,
                                                  sauce::ImGuiRenderer* imguiRenderer) {
        commandBuffers[frameIndex].begin({});

        transitionImageLayout(imageIndex, vk::ImageLayout::eUndefined,
                              vk::ImageLayout::eColorAttachmentOptimal, {},
                              vk::AccessFlagBits2::eColorAttachmentWrite,
                              vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                              vk::PipelineStageFlagBits2::eColorAttachmentOutput);

        vk::ClearValue clearColor = vk::ClearColorValue{0.1f, 0.1f, 0.1f, 1.0f};
        vk::RenderingAttachmentInfo attachmentInfo = {
            .imageView = pSwapChain->getImageViews()[imageIndex],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor,
        };

        vk::RenderingInfo renderingInfo{
            .renderArea =
                {
                    .offset = {0, 0},
                    .extent = pSwapChain->getExtent(),
                },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentInfo,
        };

        commandBuffers[frameIndex].beginRendering(renderingInfo);

        commandBuffers[frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, **pPipeline);
        commandBuffers[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                      pPipeline->getLayout(), 0,
                                                      *descriptorSets[frameIndex], nullptr);

        commandBuffers[frameIndex].setViewport(
            0, vk::Viewport(0.0f, 0.0f, static_cast<float>(pSwapChain->getExtent().width),
                            static_cast<float>(pSwapChain->getExtent().height), 0.0f, 1.0f));
        commandBuffers[frameIndex].setScissor(
            0, vk::Rect2D(vk::Offset2D(0, 0), pSwapChain->getExtent()));

        // Render all meshes
        for (const auto& meshData : meshBuffers) {
            commandBuffers[frameIndex].bindVertexBuffers(0, *meshData.vertexBuffer, {0});
            commandBuffers[frameIndex].bindIndexBuffer(*meshData.indexBuffer, 0,
                                                       vk::IndexType::eUint32);
            commandBuffers[frameIndex].drawIndexed(meshData.indexCount, 1, 0, 0, 0);
        }

        // Render ImGui overlay
        if (imguiRenderer) {
            imguiRenderer->render(commandBuffers[frameIndex], imageIndex);
        }

        commandBuffers[frameIndex].endRendering();

        transitionImageLayout(imageIndex, vk::ImageLayout::eColorAttachmentOptimal,
                              vk::ImageLayout::ePresentSrcKHR,
                              vk::AccessFlagBits2::eColorAttachmentWrite, {},
                              vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                              vk::PipelineStageFlagBits2::eBottomOfPipe);

        commandBuffers[frameIndex].end();
    }

    void ModelViewerRenderer::updateUniformBuffer(uint32_t curImage, const sauce::Camera& camera,
                                                  float modelRotationAngle) {
        sauce::UniformBufferObject ubo{
            .model = glm::rotate(glm::mat4(1.0f), modelRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f)),
            .view = camera.getViewMatrix(),
            .proj = camera.getProjectionMatrix(),
        };

        // Flip Y coordinate of projection matrix (Vulkan uses inverted Y compared to OpenGL)
        ubo.proj[1][1] *= -1;

        memcpy(uniformBuffersMapped[curImage], &ubo, sizeof(ubo));
    }

    void ModelViewerRenderer::drawFrame(const sauce::LogicalDevice& logicalDevice,
                                        const sauce::Camera& camera, float modelRotationAngle,
                                        sauce::ImGuiRenderer* imguiRenderer) {
        // Wait for the previous frame to finish rendering
        auto fenceResult =
            logicalDevice->waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
        if (fenceResult != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to wait for fence!");
        }

        // Request the next available image from the swap chain
        auto [result, imageIndex] =
            (*pSwapChain)
                ->acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error("Failed to acquire swap chain image");
        }

        // Reset the fence for the next frame
        logicalDevice->resetFences(*inFlightFences[frameIndex]);

        // Reset and record the command buffer
        commandBuffers[frameIndex].reset();
        recordCommandBuffer(imageIndex, imguiRenderer);

        // Update the uniform buffer
        updateUniformBuffer(frameIndex, camera, modelRotationAngle);

        // Submit command buffer
        vk::PipelineStageFlags waitDestinationStageMask{
            vk::PipelineStageFlagBits::eColorAttachmentOutput};
        const vk::SubmitInfo submitInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*presentCompleteSemaphores[frameIndex],
            .pWaitDstStageMask = &waitDestinationStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &*commandBuffers[frameIndex],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*renderFinishedSemaphores[imageIndex],
        };

        pQueue->submit(submitInfo, *inFlightFences[frameIndex]);

        // Present the image
        const vk::PresentInfoKHR presentInfoKHR{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
            .swapchainCount = 1,
            .pSwapchains = &***pSwapChain,
            .pImageIndices = &imageIndex,
        };

        result = pQueue->presentKHR(presentInfoKHR);
        if (result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to present swap chain image!");
        }

        frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }

} // namespace sauce
