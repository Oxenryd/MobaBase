#include "VulkanContext.hpp"
#include "Engine.h"

void VulkanContext::draw(void* rendCtx) {

	if (isPendingExit()) {
		vkDeviceWaitIdle(m_vkDevice);
		return;
	}

	auto* ctx = static_cast<RenderContext*>(rendCtx);
	auto& frame = frameSync[currentFrame];

	// Wait for previous frame fence 
	vkWaitForFences(m_vkDevice, 1, &frame.inFlight, VK_TRUE, UINT64_MAX);

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

	// Release the fence
	vkResetFences(m_vkDevice, 1, &frame.inFlight);

	//Begin command buffer
	vkResetCommandBuffer(frame.cmdBuffer, 0);
	recordCommandBuffer(frame.cmdBuffer, currentFrame);


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
	// Sort the draw commands
	//std::sort(drawCmds.begin(), drawCmds.end(), [](const MeshDrawCommand& a, const MeshDrawCommand& b) {
	//return std::tie(a.priority, a.material->base->pipelineId) <
	//	std::tie(b.priority, b.material->base->pipelineId);
	//		  });

	VkPipeline lastPipeline = VK_NULL_HANDLE;
	Material* lastMaterial = nullptr;
	uint32_t lastDescCount = 0;
	VkDescriptorSet lastSets[4]{ nullptr, nullptr, nullptr, nullptr };
	size_t firstSet = 0;
	size_t setCount = 0;
	bool pendingRebind = false;


	// Update CameraData cBuffer
	void* mappedData = nullptr;
	vkMapMemory(m_vkDevice, camDataMemory, 0, sizeof(CameraData), 0, &mappedData);
	memcpy(mappedData, &Engine::getInstance()->mainCamera()->cameraData(), sizeof(CameraData)); //memcpy(mappedData, &camData, sizeof(CameraData));
	vkUnmapMemory(m_vkDevice, camDataMemory);

	// Check the draw commands and issue binds and draw calls
	for (const auto& cmd : drawCmds) {
		Material* matBase = RenderManager::getInstance()->getMaterial(cmd.materialIndex);
		MaterialInstance& matInstance = matBase->instances[cmd.instanceIndex];

		// Bind pipeline if changed
		if (matBase->pipeline != lastPipeline) {
			vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, matBase->pipeline);
			lastPipeline = matBase->pipeline;

			//// Get sets
			//for (const auto& [setIndex, layoutKey] : matBase->descriptorSetLayoutKeys) {
			//	// Use layoutKey to get VkDescriptorSetLayout (already cached)
			//	const auto layoutIt = descSetLayoutCache.find(layoutKey);
			//	assert(layoutIt != descSetLayoutCache.end());
			//	VkDescriptorSetLayout layout = layoutIt->second;

			//	// Create a bound descriptor key (layout + bound resources)
			//	BoundDescriptorKey boundKey;
			//	boundKey.layout = layout;
			//	for (auto& binding : matInstance.resourceBindings()) {
			//		boundKey.bindings.push_back(binding);
			//	}

			//	auto setIt = descriptorSetCache.find(boundKey);
			//	if (setIt != descriptorSetCache.end()) {
			//		if (lastDescriptors[setIndex] != setIt->second) {
			//			lastDescriptors[setIndex] = setIt->second;
			//			pendingRebind = true;
			//		}
			//		
			//	}
			//}
			auto& setList = matInstance.descriptorSets() != nullptr ? *matInstance.descriptorSets() : matBase->defaultDescriptors;
			for (auto& set : setList) {
				if (lastSets[set.first] != set.second) {
					if (!pendingRebind) {
						firstSet = set.first;
						setCount = 0;
						pendingRebind = true;
					}
					lastSets[set.first] = set.second;
					setCount++;
				}
			}

			if (pendingRebind) {
				vkCmdBindDescriptorSets(frame.cmdBuffer,
										VK_PIPELINE_BIND_POINT_GRAPHICS,
										matBase->pipelineLayout,
										firstSet,
										setCount,
										lastSets,
										0, nullptr);
			}
		}

		auto scene = Engine::getInstance()->getScene(cmd.sceneIndex);

		// Push
		BaseMatPush push{};
		push.matrixIndex = UINT32_INVALID;
		push.modelToWorld = scene->registry().get<TransformComponent>(cmd.entityId).trs();
		vkCmdPushConstants(frame.cmdBuffer, matBase->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
						   0, sizeof(BaseMatPush), &push);

		
		auto& submesh = scene->sceneRender().getSubMeshes()[cmd.submeshOffset];
		auto vertexOffset = submesh.vertexOffset;
		auto indexOffset = submesh.indexOffset;
		auto indexCount = submesh.indexCount;

		vkCmdDrawIndexed(frame.cmdBuffer, indexCount, 1, indexOffset, vertexOffset, 0);
	}

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
	VkSemaphore signalSemaphores[] = { imageRenderDone[imageIndex] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// Submit command buffer
	if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, frame.inFlight) != VK_SUCCESS) {
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
	currentFrame = (currentFrame + 1) % VULKAN_MAX_FRAMES_IN_FLIGHT;
}