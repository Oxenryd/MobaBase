#include "VulkanContext.hpp"
#include "Engine.h"
#include <format>

void VulkanContext::draw(void* rendCtx) {

	if (isPendingExit()) {
		vkDeviceWaitIdle(m_vkDevice);
		return;
	}

	auto* ctx = static_cast<RenderContext*>(rendCtx);
	auto& frame = frameSync[currentFrame];

	VkResult fenceStatus = vkGetFenceStatus(m_vkDevice, frame.inFlight);
	printf("Frame %d: Fence status before wait: %d (VK_SUCCESS=%d, VK_NOT_READY=%d)\n",
		   currentFrame, fenceStatus, VK_SUCCESS, VK_NOT_READY);


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


	// Wait for previous frame fence 
	VkResult waitResult = vkWaitForFences(m_vkDevice, 1, &frame.inFlight, VK_TRUE, UINT64_MAX);
	printf("Frame %d: Fence wait result: %d\n", currentFrame, waitResult);
	if (waitResult != VK_SUCCESS) {
		printf("ERROR: Fence wait failed!\n");
		return;
	}
	vkResetFences(m_vkDevice, 1, &frame.inFlight);

	//Begin command buffer
	vkResetCommandBuffer(frame.cmdBuffer, 0);
	recordCommandBuffer(frame.cmdBuffer, currentFrame);

	//VkCommandBufferBeginInfo beginInfo{};
	//beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//beginInfo.flags = 0;
	//beginInfo.pInheritanceInfo = nullptr;
	//vkBeginCommandBuffer(frame.cmdBuffer, &beginInfo);

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
	uint32_t drawCount = 0;
	for (const auto& cmd : drawCmds) {
		Material* matBase = RenderManager::getInstance()->getMaterial(cmd.materialIndex);
		MaterialInstance& matInstance = matBase->instances[cmd.instanceIndex];

		// Bind pipeline if changed
		if (matBase->pipeline != lastPipeline) {
			vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, matBase->pipeline);
			lastPipeline = matBase->pipeline;

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
		push.materialIndex = cmd.materialIndex;
		push.modelToWorld = scene->registry().get<TransformComponent>(cmd.entityId).trs();
		vkCmdPushConstants(frame.cmdBuffer, matBase->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
						   0, sizeof(BaseMatPush), &push);

		
		auto& submesh = scene->sceneRender().getSubMeshes()[cmd.submeshOffset];
		auto vertexOffset = submesh.vertexOffset;
		auto indexOffset = submesh.indexOffset;
		auto indexCount = submesh.indexCount;

		vkCmdDrawIndexed(frame.cmdBuffer, indexCount, 1, indexOffset, vertexOffset, 0);
		drawCount++;
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

	// Signal done
	VkSemaphore signalSemaphores[] = { frame.renderFinished };
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
	currentFrame = (currentFrame + 1) % VULKAN_MAX_FRAMES_IN_FLIGHT;
}