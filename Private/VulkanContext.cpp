#include "Engine.h"
#include "VulkanContext.hpp"
#include <format>
#include <chrono>
#include <set>

INLINE MeshDrawCommand VulkanContext::subMeshEntity_to_drawCommand(SceneBase* scene, ArenaRegistry& reg, entt::entity entity) {
	auto& subMeshComp = reg.get<SubMeshComponent>(entity);
	SubMeshData& subMesh = scene->sceneRender().getSubMeshes()[subMeshComp.subMeshIndex];
	MeshDrawCommand cmd{};
	cmd.instanceIndex = subMesh.instanceIndex;
	cmd.materialIndex = subMesh.materialIndex;
	cmd.priority = 1.0f; //TODO
	cmd.sceneIndex = scene->sceneIndex();
	cmd.submeshOffset = subMeshComp.subMeshIndex;
	cmd.subMeshEntity = entity;

	return cmd;
}

void VulkanContext::draw(const DrawContext& ctx) {

	if (isPendingExit()) {
		vkDeviceWaitIdle(m_vkDevice);
		return;
	}

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
	viewport.x = ctx.vPortPos[0];
	viewport.y = ctx.vPortPos[1];
	viewport.width = ctx.vPortSize[0] < 0 ? static_cast<float>(swapchainExtent.width) : ctx.vPortSize[0];
	viewport.height = ctx.vPortSize[1] < 0 ? static_cast<float>(swapchainExtent.height) : ctx.vPortSize[1];
	viewport.minDepth = ctx.vPortMinDepth;
	viewport.maxDepth = ctx.vPortMaxDepth;
	vkCmdSetViewport(frame.cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { ctx.sciOffset[0], ctx.sciOffset[1] };
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
	Frustum& f = mainCam->getFrustum();

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




	// Find the draw commands
	struct BoundedInstanceData
	{
		std::vector<InstanceData> instances;
		AABB bounds;
	};
	static std::vector<MeshDrawCommand> drawCmds;
	static std::unordered_map<uint32_t, BoundedInstanceData> submeshDrawInstanceData[VULKAN_FRAMES_IN_FLIGHT];
	static std::vector<ModelTransform> collectedMatrices[VULKAN_FRAMES_IN_FLIGHT];
	static std::unordered_set<uint32_t> submeshKeysWithMultipleInstances[VULKAN_FRAMES_IN_FLIGHT];
	collectedMatrices[currentFrame].clear();
	submeshDrawInstanceData[currentFrame].clear();
	drawCmds.clear();
	submeshKeysWithMultipleInstances[currentFrame].clear();
	for (auto& scene : Engine::getInstance()->getActiveScenes()) {

		if (collectedMatrices[currentFrame].capacity() < scene->transformSystem().modelTransforms().size())
			collectedMatrices[currentFrame].reserve(collectedMatrices[currentFrame].capacity() + scene->transformSystem().modelTransforms().size());

		collectedMatrices[currentFrame].insert(collectedMatrices[currentFrame].end(),
								 scene->transformSystem().modelTransforms().begin(),
								 scene->transformSystem().modelTransforms().end());

		auto& reg = scene->registry();
		for (auto& entity : scene->cullResults.visibleEntities) {
			auto newDrawCmd = subMeshEntity_to_drawCommand(scene, reg, entity);
			auto it = submeshDrawInstanceData[currentFrame].find(newDrawCmd.submeshOffset);
			if (it == submeshDrawInstanceData[currentFrame].end()) {
				drawCmds.push_back(newDrawCmd);
				submeshDrawInstanceData[currentFrame].insert({ newDrawCmd.submeshOffset, BoundedInstanceData() });
				
				auto& transComp = scene->registry().get<TransformComponent>(newDrawCmd.subMeshEntity);
				InstanceData instData{};
				instData.matInstanceIndex = newDrawCmd.materialIndex;				
				instData.matrixIndex = transComp.dataIndex;
				submeshDrawInstanceData[currentFrame][newDrawCmd.submeshOffset].instances.push_back(instData);
				BoundingVolume bVol = BoundingVolume{ &scene->registry() , newDrawCmd.subMeshEntity};
				submeshDrawInstanceData[currentFrame][newDrawCmd.submeshOffset].bounds.merge(bVol.getCoarseAABB());

			} else {
				auto& transComp = scene->registry().get<TransformComponent>(newDrawCmd.subMeshEntity);
				InstanceData instData{};
				instData.matInstanceIndex = newDrawCmd.materialIndex;
				instData.matrixIndex = transComp.dataIndex;
				submeshDrawInstanceData[currentFrame][newDrawCmd.submeshOffset].instances.push_back(instData);
				BoundingVolume bVol = BoundingVolume{ &scene->registry() , newDrawCmd.subMeshEntity };
				submeshDrawInstanceData[currentFrame][newDrawCmd.submeshOffset].bounds.merge(bVol.getCoarseAABB());
				
				submeshKeysWithMultipleInstances[currentFrame].insert(it->first);
			}
		}
	}
	std::sort(drawCmds.begin(), drawCmds.end());


	// push matrices to GPU
	{
		VkResult vkResult{};
		VkDeviceSize bufferSize = sizeof(ModelTransform) * collectedMatrices[currentFrame].size();
		VkBufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.size = bufferSize;
		if (modelTransforms_lastBufferSize[currentFrame] != bufferSize) {
			vkDestroyBuffer(m_vkDevice, matBuf_modelTransforms[currentFrame], nullptr);
			modelTransforms_lastBufferSize[currentFrame] = bufferSize;

			if (matDevMem_modelTransforms[currentFrame] != VK_NULL_HANDLE)
				vkFreeMemory(m_vkDevice, matDevMem_modelTransforms[currentFrame], nullptr);

			vkResult = vkCreateBuffer(m_vkDevice, &createInfo, nullptr, &matBuf_modelTransforms[currentFrame]);
			VkMemoryRequirements bufferMemReq{};
			vkGetBufferMemoryRequirements(m_vkDevice, matBuf_modelTransforms[currentFrame], &bufferMemReq);
			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = bufferMemReq.size;
			allocInfo.memoryTypeIndex = findMemoryType(
				bufferMemReq.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				m_phyDevice
			);
			vkResult = vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &matDevMem_modelTransforms[currentFrame]);
			vkResult = vkBindBufferMemory(m_vkDevice, matBuf_modelTransforms[currentFrame], matDevMem_modelTransforms[currentFrame], 0);
		
			VkDescriptorSet matricesSet = bindingToDescriptorSet[{MAT_MODELMATRICES_BIND, MAT_MODELMATRICES_SET, 0}][currentFrame];
			std::vector<VkWriteDescriptorSet> descriptorWrites;
			VkDescriptorBufferInfo modelMatricesBufferInfo{};
			modelMatricesBufferInfo.buffer = matBuf_modelTransforms[currentFrame];
			modelMatricesBufferInfo.offset = 0;
			modelMatricesBufferInfo.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet matrixDataWrite{};
			matrixDataWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			matrixDataWrite.dstSet = matricesSet;
			matrixDataWrite.dstBinding = MAT_MODELMATRICES_BIND;
			matrixDataWrite.dstArrayElement = 0;
			matrixDataWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			matrixDataWrite.descriptorCount = 1;
			matrixDataWrite.pBufferInfo = &modelMatricesBufferInfo;
			vkUpdateDescriptorSets(m_vkDevice, 1, &matrixDataWrite, 0, nullptr);
		
		}

		void* data;
		vkMapMemory(m_vkDevice, matDevMem_modelTransforms[currentFrame], 0, bufferSize, 0, &data);
		memcpy(data, collectedMatrices[currentFrame].data(), (size_t)bufferSize);
		vkUnmapMemory(m_vkDevice, matDevMem_modelTransforms[currentFrame]);
	}


	// update instanceData on gpu
	static std::vector<InstanceData> instDataTemp[VULKAN_FRAMES_IN_FLIGHT];
	instDataTemp[currentFrame].clear();
	for (auto& key : submeshKeysWithMultipleInstances[currentFrame]) {


		for (auto& instData : submeshDrawInstanceData[currentFrame][key].instances)
			instDataTemp[currentFrame].push_back(instData);


		// Copy instance data to GPU
		{
			VkResult vkResult{};
			VkDeviceSize bufferSize = sizeof(InstanceData) * instDataTemp[currentFrame].size();
			VkBufferCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.size = bufferSize;
			if (instancesIndex_lastBufferSize[currentFrame] != bufferSize) {
				vkDestroyBuffer(m_vkDevice, instanceIndexBuffer[currentFrame], nullptr);
				instancesIndex_lastBufferSize[currentFrame] = bufferSize;

				if (instanceIndexMemory[currentFrame] != VK_NULL_HANDLE)
					vkFreeMemory(m_vkDevice, instanceIndexMemory[currentFrame], nullptr);

				vkResult = vkCreateBuffer(m_vkDevice, &createInfo, nullptr, &instanceIndexBuffer[currentFrame]);
				VkMemoryRequirements bufferMemReq{};
				vkGetBufferMemoryRequirements(m_vkDevice, instanceIndexBuffer[currentFrame], &bufferMemReq);
				VkMemoryAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = bufferMemReq.size;
				allocInfo.memoryTypeIndex = findMemoryType(
					bufferMemReq.memoryTypeBits,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					m_phyDevice
				);
				vkResult = vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &instanceIndexMemory[currentFrame]);
				vkResult = vkBindBufferMemory(m_vkDevice, instanceIndexBuffer[currentFrame], instanceIndexMemory[currentFrame], 0);

				VkDescriptorSet instanceIndexSet = bindingToDescriptorSet[{MAT_BASE_INSTANCES_DATA_BIND, MAT_BASE_INSTANCES_DATA_SET, 0}][currentFrame];
				std::vector<VkWriteDescriptorSet> descriptorWrites;
				VkDescriptorBufferInfo instanceIndexBufferInfo{};
				instanceIndexBufferInfo.buffer = instanceIndexBuffer[currentFrame];
				instanceIndexBufferInfo.offset = 0;
				instanceIndexBufferInfo.range = VK_WHOLE_SIZE;
				VkWriteDescriptorSet instanceDataWrite{};
				instanceDataWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				instanceDataWrite.dstSet = instanceIndexSet;
				instanceDataWrite.dstBinding = MAT_BASE_INSTANCES_DATA_BIND;
				instanceDataWrite.dstArrayElement = 0;
				instanceDataWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				instanceDataWrite.descriptorCount = 1;
				instanceDataWrite.pBufferInfo = &instanceIndexBufferInfo;
				vkUpdateDescriptorSets(m_vkDevice, 1, &instanceDataWrite, 0, nullptr);

			}

			void* data;
			vkMapMemory(m_vkDevice, instanceIndexMemory[currentFrame], 0, bufferSize, 0, &data);
			memcpy(data, instDataTemp[currentFrame].data(), (size_t)bufferSize);
			vkUnmapMemory(m_vkDevice, instanceIndexMemory[currentFrame]);

		}

	}

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


	// Begin Render Pass
	VkClearValue clearValues[2] = {};
	clearValues[0].color = { ctx.clearColor[0], ctx.clearColor[1], ctx.clearColor[2], ctx.clearColor[3] };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = rendPasses[0];//rendPasses[ctx.renderPassIndex];
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
		auto instanceSize = submeshDrawInstanceData[currentFrame][cmd.submeshOffset].instances.size();
		if (instanceSize > 1) {
			auto& aabb = submeshDrawInstanceData[currentFrame][cmd.submeshOffset].bounds;
			if (!MMath::aabbVisible(aabb.min, aabb.max, f))
				continue;	
		}

		auto* scene = Engine::getInstance()->getScene(cmd.sceneIndex);

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

		
		if (instanceSize > 1) {
			
			// Push for instanced
			BaseMatPush push{};
			push.flags = (uint32_t)BaseMatPushFlags::Instanced;
			vkCmdPushConstants(frame.cmdBuffer, pipelineLayouts[matBase->pipelineLayoutId], VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
							   0, sizeof(BaseMatPush), &push);

			auto& submesh = scene->sceneRender().getSubMeshes()[cmd.submeshOffset];
			auto vertexOffset = submesh.vertexOffset;
			auto indexOffset = submesh.indexOffset;
			auto indexCount = submesh.indexCount;

			vkCmdDrawIndexed(frame.cmdBuffer, indexCount, static_cast<uint32_t>(instanceSize), indexOffset, vertexOffset, 0);
			drawCount++;


		} else {
			// Push
			BaseMatPush push{};
			push.flags = 0;
			push.matInstanceIndex = cmd.instanceIndex;
			auto& transComp = scene->registry().get<TransformComponent>(cmd.subMeshEntity);
			push.modelToWorld = scene->transformSystem().modelTransforms()[transComp.dataIndex];
			vkCmdPushConstants(frame.cmdBuffer, pipelineLayouts[matBase->pipelineLayoutId], VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
							   0, sizeof(BaseMatPush), &push);


			auto& submesh = scene->sceneRender().getSubMeshes()[cmd.submeshOffset];
			auto vertexOffset = submesh.vertexOffset;
			auto indexOffset = submesh.indexOffset;
			auto indexCount = submesh.indexCount;

			vkCmdDrawIndexed(frame.cmdBuffer, indexCount, 1, indexOffset, vertexOffset, 0);
			drawCount++;
		}
	}


	// Debug AABBs
	bool anyAabbsDrawn = false;
	Material* shapeMat;
	uint32_t shapeDraws = 0;
	for (auto* scene : Engine::getInstance()->getActiveScenes()) {

		if (!scene->sceneRender().drawCoarseAbbs()) 
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
		
		if (scene->sceneRender().drawOccluders()) {
			for (auto& entity : scene->cullResults.activeOccluders) {
				auto [bound, transform] = scene->registry().get<BoundingVolumeComponent, TransformComponent>(entity);
				AABB worldBox = scene->boundingSystem().aabbs()[bound.coarseIndexWorld];
				ShapePush shapePush{};
				shapePush.modelToWorld = Transform::composeTRS(&scene->registry(), entity);
				shapePush.color = { 1.0f, 0.01f, 0.02f, 0.01f };
				shapePush.rotation = glm::quat();
				shapePush.aabb = { worldBox.min, worldBox.max };
				shapePush.drawNumber = shapeDraws++;

				vkCmdPushConstants(frame.cmdBuffer, pipelineLayouts[shapeMat->pipelineLayoutId], VK_SHADER_STAGE_VERTEX_BIT,
								   0, sizeof(ShapePush), &shapePush);

				vkCmdDrawIndexed(frame.cmdBuffer, 36, 1, 0, 0, 0);
				drawCount++;
			}
		} else if (scene->sceneRender().drawNodes()) {

			for (size_t n = 0; n < scene->bvhSystem().bvh().getNodeCount(0); ++n) {
				auto& node = scene->bvhSystem().bvh().nodes[0][n];
				AABB& box = node.bounds;
				ShapePush shapePush{};
				shapePush.modelToWorld = glm::mat4{ 1 };
				shapePush.color = { 1.0f, 0.01f, 1.00f, 0.01f };
				shapePush.rotation = glm::quat();
				shapePush.aabb = { box.min, box.max };
				shapePush.drawNumber = shapeDraws++;

				vkCmdPushConstants(frame.cmdBuffer, pipelineLayouts[shapeMat->pipelineLayoutId], VK_SHADER_STAGE_VERTEX_BIT,
								   0, sizeof(ShapePush), &shapePush);

				vkCmdDrawIndexed(frame.cmdBuffer, 36, 1, 0, 0, 0);
				drawCount++;
			}
			
		} else {
			auto view = scene->registry().view<BoundingVolumeComponent, TransformComponent>();
			for (auto [entity, bound, transform] : view.each()) {

				AABB worldBox = scene->boundingSystem().aabbs()[bound.coarseIndexWorld];
				ShapePush shapePush{};
				shapePush.modelToWorld = Transform::composeTRS(&scene->registry(), entity);
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
