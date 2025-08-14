#include "VulkanContext.hpp"
#include "Engine.h"
#include <format>
#include <chrono>

void VulkanContext::draw(void* rendCtx) {

	if (isPendingExit()) {
		vkDeviceWaitIdle(m_vkDevice);
		return;
	}

	auto* ctx = static_cast<RenderContext*>(rendCtx);
	auto& frame = frameSync[currentFrame];


	// Wait for previous frame fence 
	vkWaitForFences(m_vkDevice, 1, &frame.inFlight, VK_TRUE, UINT64_MAX);
	vkResetFences(m_vkDevice, 1, &frame.inFlight);

	// Acquire swapchain image	 
	uint32_t imageIndex = 0;
	auto acquireResult = vkAcquireNextImageKHR(
		m_vkDevice, swapchain,
		UINT64_MAX, frame.imageAvailable,
		VK_NULL_HANDLE, &imageIndex);


	if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || pendingResize) {
		recreateSwapchain();
		return;
	}

	//Begin command buffer
	vkResetCommandBuffer(frame.cmdBuffer, 0);
	recordCommandBuffer(frame.cmdBuffer, currentFrame);

	// Set Dynamic state
	VkViewport viewport{};
	viewport.x = ctx->vPortPos[0];
	viewport.y = ctx->vPortPos[1];
	viewport.width = ctx->vPortSize[0] < 0 ? static_cast<float>(swapchainExtent.width) : ctx->vPortSize[0];
	viewport.height = ctx->vPortSize[1] < 0 ? static_cast<float>(swapchainExtent.height) : ctx->vPortSize[1];
	viewport.minDepth = ctx->vPortMinDepth;
	viewport.maxDepth = ctx->vPortMaxDepth;
	vkCmdSetViewport(frame.cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { ctx->sciOffset[0], ctx->sciOffset[1] };
	scissor.extent = swapchainExtent;
	vkCmdSetScissor(frame.cmdBuffer, 0, 1, &scissor);

	// Camera
	auto mainCam = Engine::getInstance()->mainCamera();
	auto& camData = mainCam->cameraData();
	camData.numLights = static_cast<uint32_t>(lightsData.size());
	camData.screenSize = { viewport.width, viewport.height };
	camData.invScreenSize = { 1.0f / camData.screenSize.x, 1.0f / camData.screenSize.y };
	camData.clustersX = VULKAN_LIGHT_CLUSTERS_X;
	camData.clustersY = VULKAN_LIGHT_CLUSTERS_Y;
	camData.clustersZ = VULKAN_LIGHT_CLUSTERS_Z;
	Frustum f = mainCam->getFrustum();

	// Update CameraData cBuffer
	void* mappedData = nullptr;
	vkMapMemory(m_vkDevice, camDataMemory[currentFrame], 0, sizeof(CameraData), 0, &mappedData);
	memcpy(mappedData, &camData, sizeof(CameraData));
	vkUnmapMemory(m_vkDevice, camDataMemory[currentFrame]);


	// Dispatch light cluster CS
	vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, lightCluster_pipeline);
	vkCmdBindDescriptorSets(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
							lightCluster_pipelineLayout, 0, 3, lightCluster_descSets[currentFrame].data(),
							0, nullptr);

	// Compute proper group counts from clusters and shader local size
	constexpr uint32_t LOCAL_X = VULKAN_LIGHT_CLUSTER_THREADS_X; // matches [numthreads(…)]
	constexpr uint32_t LOCAL_Y = VULKAN_LIGHT_CLUSTER_THREADS_Y;
	constexpr uint32_t LOCAL_Z = VULKAN_LIGHT_CLUSTER_THREADS_Z;
	auto ceilDiv = [](uint32_t a, uint32_t b) { return (a + b - 1) / b; };

	const uint32_t groupsX = ceilDiv(camData.clustersX, LOCAL_X);
	const uint32_t groupsY = ceilDiv(camData.clustersY, LOCAL_Y);
	const uint32_t groupsZ = ceilDiv(camData.clustersZ, LOCAL_Z);

	vkCmdDispatch(frame.cmdBuffer, VULKAN_LIGHT_CLUSTERS_X, VULKAN_LIGHT_CLUSTERS_Y, VULKAN_LIGHT_CLUSTERS_Z);

	// Barrier: make compute writes visible to graphics reads
#if VK_HEADER_VERSION >= 230  // assuming you have 1.3 / sync2
	VkMemoryBarrier2 memBarrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER_2 };
	memBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT; // writes in CS
	memBarrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	memBarrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT;

	VkDependencyInfo dep{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dep.memoryBarrierCount = 1;
	dep.pMemoryBarriers = &memBarrier;

	vkCmdPipelineBarrier2(frame.cmdBuffer, &dep);
#else
	VkMemoryBarrier mem{};
	mem.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	mem.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	mem.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;

	vkCmdPipelineBarrier(
		cmd,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		1, &mem,
		0, nullptr,
		0, nullptr);
#endif




	//// In updateLightsData(), after vkBindBufferMemory
	//void* mappedData2 = nullptr;
	//vkMapMemory(m_vkDevice, lightsMemory[currentFrame], 0, sizeof(GPULight) * lightsData.size(), 0, &mappedData2);
	//memcpy(mappedData2, lightsData.data(), sizeof(GPULight) * lightsData.size());
	//vkUnmapMemory(m_vkDevice, lightsMemory[currentFrame]);
	//// DEBUG: Verify the data
	//GPULight* uploadedLights = (GPULight*)mappedData2;
	//for (size_t j = 0; j < lightsData.size(); j++) {
	//	printf("Light %zu: type=%d, pos=(%f,%f,%f) flags:%zu\n", j, uploadedLights[j].type,
	//		   uploadedLights[j].positionVS.x, uploadedLights[j].positionVS.y, uploadedLights[j].positionVS.z,
	//		   uploadedLights[j].flags);
	//}

	












	// Begin Render Pass
	VkClearValue clearValues[2] = {};
	clearValues[0].color = { ctx->clearColor[0], ctx->clearColor[1], ctx->clearColor[2], ctx->clearColor[3] };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = rendPasses[ctx->renderPassIndex];
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapchainExtent;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass(frame.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// bind vertex buffer
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(frame.cmdBuffer, 0, 1, &vertexBuffer, offsets);
	vkCmdBindIndexBuffer(frame.cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);


	static std::vector<MeshDrawCommand> drawCmds;
	drawCmds.clear();
	for (auto& scene : Engine::getInstance()->getActiveScenes()) {
		for (auto& cmd : scene->sceneRender().persistentDrawCommands()) {
			drawCmds.push_back(cmd);
		}
	}
	std::sort(drawCmds.begin(), drawCmds.end());

	//VkPipeline lastPipeline = VK_NULL_HANDLE;
	uint32_t lastPipelineIndex = UINT32_INVALID - 1;
	Material* lastMaterial = nullptr;
	uint32_t lastDescCount = 0;
	uint32_t lastMatIndex = UINT32_INVALID;
	VkDescriptorSet lastSets[4]{ nullptr, nullptr, nullptr, nullptr };
	size_t firstSet = 0;
	size_t setCount = 0;
	bool pendingRebind = false;


	// Check the draw commands and issue binds and draw calls
	uint32_t drawCount = 0;
	uint32_t pipelinesCount = 0;
	for (const auto& cmd : drawCmds) {

		
		auto* scene = Engine::getInstance()->getScene(cmd.sceneIndex);

		// Frustum culling
		const auto [bound, transform] = scene
			->registry().try_get<BoundingVolumeComponent, TransformComponent>(cmd.subMeshEntity);
		if (transform) {
			auto world = scene->boundingSystem().aabbs()[bound->coarseIndexWorld];
			
			//coarse.center += transform->position;
			if (!MMath::aabbVisible(world.min, world.max, f))
				continue;
		}



		Material* matBase = RenderManager::getInstance()->getMaterial(cmd.materialIndex);

		if (cmd.materialIndex != lastMatIndex) {
			// Bind pipeline if changed
			if (matBase->pipelineId != lastPipelineIndex) {
				vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[matBase->pipelineId]);
				lastPipelineIndex = matBase->pipelineId;
				pipelinesCount++;
			}

			// check descriptor sets
			auto& setList = matBase->dependentDescriptorBindings;
			for (auto& combo : setList) {
				auto& descSet = bindingToDescriptorSet[combo];
				if (lastSets[combo.set] != descSet[currentFrame]) {
					if (!pendingRebind) {
						firstSet = combo.set;
						setCount = 0;
						pendingRebind = true;
					}
					lastSets[combo.set] = descSet[currentFrame];
					setCount++;
				}
			}


			if (pendingRebind) {
				vkCmdBindDescriptorSets(frame.cmdBuffer,
										VK_PIPELINE_BIND_POINT_GRAPHICS,
										pipelineLayouts[matBase->pipelineLayoutId],
										firstSet,
										setCount,
										lastSets,
										0, nullptr);
				pendingRebind = false;
			}
			lastMatIndex = cmd.materialIndex;
		}

		// Push
		BaseMatPush push{};
		push.matrixIndex = UINT32_INVALID;
		push.matInstanceIndex = cmd.instanceIndex;
		push.modelToWorld = scene->registry().get<TransformComponent>(cmd.entityId).trs();
		vkCmdPushConstants(frame.cmdBuffer, pipelineLayouts[matBase->pipelineLayoutId], VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
						   0, sizeof(BaseMatPush), &push);

		
		auto& submesh = scene->sceneRender().getSubMeshes()[cmd.submeshOffset];
		auto vertexOffset = submesh.vertexOffset;
		auto indexOffset = submesh.indexOffset;
		auto indexCount = submesh.indexCount;

		vkCmdDrawIndexed(frame.cmdBuffer, indexCount, 1, indexOffset, vertexOffset, 0);
		drawCount++;
	}


	// Debug AABBs
	bool anyAabbsDrawn = false;
	Material* shapeMat;
	uint32_t shapeDraws = 0;
	for (auto* scene : Engine::getInstance()->getActiveScenes()) {
		if (!scene->sceneRender().drawAbbs())
			continue;

		if (!anyAabbsDrawn) {

			void* shapeIndexData = nullptr;
			vkMapMemory(m_vkDevice, shapeIndexMemory[currentFrame], 0, sizeof(CameraData), 0, &shapeIndexData);
			memcpy(shapeIndexData, &AaBbTriIndices, sizeof(uint32_t) * 36);
			vkUnmapMemory(m_vkDevice, shapeIndexMemory[currentFrame]);

			vkCmdBindIndexBuffer(frame.cmdBuffer, shapeIndexBuffer[currentFrame], 0, VK_INDEX_TYPE_UINT32);
			shapeMat = RenderManager::getInstance()->getMaterial("ShapeRendererMaterial");
			vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[shapeMat->pipelineId]);

			vkCmdBindDescriptorSets(frame.cmdBuffer,
									VK_PIPELINE_BIND_POINT_GRAPHICS,
									pipelineLayouts[shapeMat->pipelineLayoutId],
									0,
									1,
									&shapeRendererDescSet[currentFrame],
									0, nullptr);
			pipelinesCount++;
			setCount++;
			anyAabbsDrawn = true;
		}

		auto view = scene->registry().view<BoundingVolumeComponent, TransformComponent>();
		for (auto [entity, bound, transform] : view.each()) {

			AABB worldBox = scene->boundingSystem().aabbs()[bound.coarseIndexWorld];
			//glm::vec3 mn{ local.frontTopLeft.x,  local.backBottomRight.y, local.backBottomRight.z };
			//glm::vec3 mx{ local.backBottomRight.x, local.frontTopLeft.y,  local.frontTopLeft.z };

			ShapePush shapePush{};
			shapePush.modelToWorld = transform.trs();
			shapePush.color = { 0.02f, 1.0f, 0.02f, 0.01f };
			shapePush.rotation = glm::quat();
			shapePush.aabb = { worldBox.min, worldBox.max };
			shapePush.drawNumber = shapeDraws++;

			vkCmdPushConstants(frame.cmdBuffer, pipelineLayouts[shapeMat->pipelineLayoutId], VK_SHADER_STAGE_VERTEX_BIT,
							   0, sizeof(ShapePush), &shapePush);

			vkCmdDrawIndexed(frame.cmdBuffer, 36, 1, 0, 0, 0);
			drawCount++;
		}
	}


	m_lastDrawcallCount = drawCount;
	m_lastPipelineSwitches = pipelinesCount;

	// End Render Pass
	vkCmdEndRenderPass(frame.cmdBuffer);

	// End command buffer
	if (vkEndCommandBuffer(frame.cmdBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { frame.imageAvailable };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frame.cmdBuffer;

	// Signal done
	VkSemaphore signalSemaphores[] = { imageRenderFinished[imageIndex] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// Submit command buffer
	auto submitResult = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, frame.inFlight);
	if (submitResult != VK_SUCCESS) {
		LOGLINE(LogType::Error, LogMod::Vulkan, "failed to submit draw command buffer!");
		vkDeviceWaitIdle(m_vkDevice);
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	// Present frame
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional


	auto presentResult = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || pendingResize) {
		recreateSwapchain();
	}

	// rotate frame sync /semaphores
	currentFrame = (currentFrame + 1) % VULKAN_FRAMES_IN_FLIGHT;
}

void VulkanContext::preDraw(RenderManager* const renderMan) {

	

	// Collect pendingLight updates
	// ...
	for (auto& light : pendingLightUpdates) {
		lightsData.push_back(light);
	}

	if (!pendingLightUpdates.empty())
		updateLightsData();

	if (!lightCluster_pipelineCreated) {
		auto* lightClusterCs = renderMan->getShader(SHADER_BASE_LIGHTCLUSTER_CS);
		createLightClusterPipeline(lightClusterCs);
	}


	// Reset stuff
	pendingLightUpdates.clear();
}

void VulkanContext::postDraw() {

}
