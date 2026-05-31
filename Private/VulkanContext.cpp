#include "Engine.h"
#include "VulkanContext.hpp"
#include "Profiler.hpp"
#include "Robin_Hood.h"
#include "Mesh.hpp"
#include "Scene.h"
#include "EnabledTag.hpp"
#include "TransformSystem.hpp"
#include "Camera.hpp"
#include "SceneRenderSystem.hpp"
#include "BVH.hpp"

#include <chrono>

void VulkanContext::_drawCommandWorker(VulkanContext* _this) {

	while (true) {


		_this->m_drawCmdStartSema.acquire();

		if (!_this->m_drawRunning.load())
			return;

		size_t idx = _this->m_drawCommandBufferIndex.load(std::memory_order_acquire);
		if (idx == UINT8_INVALID)
			idx = 0;
		else
			idx = (idx + 1) % DRAW_COMMAND_BUFFER_SIZE;
	{
		// Find the draw commands
		PROFILE_SCOPE("DrawCommands");

		//submeshKeysWithMultipleInstances[currentFrame].clear();
		_this->submeshDrawInstanceData[idx].clear();
		//drawCmds[currentFrame].clear();

		for (auto& scene : Engine::getInstance()->getActiveScenes()) {

			auto sceneIndex = scene->sceneIndex();

			if (sceneIndex >= _this->drawCmds[idx].size()) {
				for (size_t i = _this->drawCmds[idx].size(); i < sceneIndex + 1; ++i)
					_this->drawCmds[idx].push_back({});
			}
			_this->drawCmds[idx][sceneIndex].clear();

			if (sceneIndex >= _this->submeshKeysWithMultipleInstances[idx].size()) {
				for (size_t i = _this->submeshKeysWithMultipleInstances[idx].size(); i < sceneIndex + 1; ++i)
					_this->submeshKeysWithMultipleInstances[idx].push_back({});
			}
			_this->submeshKeysWithMultipleInstances[idx][sceneIndex].clear();

			if (sceneIndex >= _this->submeshDrawInstanceData[idx].size()) {
				for (size_t i = _this->submeshDrawInstanceData[idx].size(); i < sceneIndex + 1; ++i)
					_this->submeshDrawInstanceData[idx].push_back({});
			}
			_this->submeshDrawInstanceData[idx][sceneIndex].clear();

			auto& reg = scene->registry();
			auto grp = reg.group<TransformComponent, BoundingVolumeComponent, EnabledTag>();
			for (auto& entity : scene->cullResults.visibleEntities) {

				auto newDrawCmd = _this->subMeshEntity_to_drawCommand(scene, reg, entity);
				//const SceneInstancePair mapIndex{ sceneIndex, newDrawCmd.submeshOffset };

				auto it = _this->submeshDrawInstanceData[idx][sceneIndex].find(newDrawCmd.submeshOffset);
				if (it == _this->submeshDrawInstanceData[idx][sceneIndex].end()) {
					_this->drawCmds[idx][sceneIndex].push_back(newDrawCmd);
					auto keyPair = _this->submeshDrawInstanceData[idx][sceneIndex]
						.insert({ newDrawCmd.submeshOffset, BoundedInstanceData(_this->instanceDataArena[idx]) });

					BoundedInstanceData& instanceData = keyPair.first->second;//submeshDrawInstanceData[currentFrame][sceneIndex][keyPair.second];
					instanceData.instances.reserve(8192);

					auto& transComp = grp.get<TransformComponent>(newDrawCmd.subMeshEntity);//scene->registry().get<TransformComponent>(newDrawCmd.subMeshEntity);
					InstanceData instData{};
					instData.matInstanceIndex = newDrawCmd.materialIndex;
					instData.matrixIndex = transComp.dataIndex;
					instanceData.instances.push_back(instData);
					//BoundingVolumeComponent& bVol = grp.get<BoundingVolumeComponent>(newDrawCmd.subMeshEntity);
					//instanceData.bounds.merge(Engine::getInstance()->
					//						  getScene(transComp.sceneIndex)->boundingSystem().cachedLocals()[bVol.coarseIndexLocal]);

				} else {
					BoundedInstanceData& instanceData = it->second;
					auto& transComp = grp.get<TransformComponent>(newDrawCmd.subMeshEntity);//auto& transComp = scene->registry().get<TransformComponent>(newDrawCmd.subMeshEntity);
					InstanceData instData{};
					instData.matInstanceIndex = newDrawCmd.materialIndex;
					instData.matrixIndex = transComp.dataIndex;
					instanceData.instances.push_back(instData);
					//BoundingVolumeComponent& bVol = grp.get<BoundingVolumeComponent>(newDrawCmd.subMeshEntity);
					//instanceData.bounds.merge(Engine::getInstance()->
					//						  getScene(transComp.sceneIndex)->boundingSystem().cachedLocals()[bVol.coarseIndexLocal]);

					_this->submeshKeysWithMultipleInstances[idx][sceneIndex].insert(newDrawCmd.submeshOffset);
				}
			}



			std::sort(_this->drawCmds[idx][sceneIndex].begin(), _this->drawCmds[idx][sceneIndex].end());

		}
	} // Profiler scope end

		_this->m_drawCommandBufferIndex.store(idx, std::memory_order_release);
		_this->m_drawCmdFinishSema.release();

	}
}

INLINE MeshDrawCommand VulkanContext::subMeshEntity_to_drawCommand(SceneBase* scene, ArenaRegistry& reg, entt::entity entity) {
	auto& subMeshComp = reg.get<MeshFilterComponent>(entity);
	MeshData& subMesh = scene->sceneRender().getSubMeshes()[subMeshComp.meshDataIndex];
	MeshDrawCommand cmd{};
	cmd.instanceIndex = subMesh.instanceIndex;
	cmd.materialIndex = subMesh.materialIndex;
	cmd.priority = 0xffff; //TODO
	cmd.sceneIndex = scene->sceneIndex();
	cmd.submeshOffset = subMeshComp.meshDataIndex;
	cmd.subMeshEntity = entity;

	return cmd;
}

void VulkanContext::draw(const DrawContext& ctx) {

	if (isPendingExit()) {
		vkDeviceWaitIdle(m_vkDevice);
		return;
	}

	auto& frame = frameSync[currentFrame];





	// // Find the draw commands
	// {
	// 	PROFILE_SCOPE("DrawCommands");
	//
	// 	//submeshKeysWithMultipleInstances[currentFrame].clear();
	// 	submeshDrawInstanceData[currentFrame].clear();
	// 	//drawCmds[currentFrame].clear();
	//
	// 	for (auto& scene : Engine::getInstance()->getActiveScenes()) {
	//
	// 		auto sceneIndex = scene->sceneIndex();
	//
	// 		if (sceneIndex >= drawCmds[currentFrame].size()) {
	// 			for (size_t i = drawCmds[currentFrame].size(); i < sceneIndex + 1; ++i)
	// 				drawCmds[currentFrame].push_back({});
	// 		}
	// 		drawCmds[currentFrame][sceneIndex].clear();
	//
	// 		if (sceneIndex >= submeshKeysWithMultipleInstances[currentFrame].size()) {
	// 			for (size_t i = submeshKeysWithMultipleInstances[currentFrame].size(); i < sceneIndex + 1; ++i)
	// 				submeshKeysWithMultipleInstances[currentFrame].push_back({});
	// 		}
	// 		submeshKeysWithMultipleInstances[currentFrame][sceneIndex].clear();
	//
	// 		if (sceneIndex >= submeshDrawInstanceData[currentFrame].size()) {
	// 			for (size_t i = submeshDrawInstanceData[currentFrame].size(); i < sceneIndex + 1; ++i)
	// 				submeshDrawInstanceData[currentFrame].push_back({});
	// 		}
	// 		submeshDrawInstanceData[currentFrame][sceneIndex].clear();
	//
	// 		auto& reg = scene->registry();
	// 		auto grp = reg.group<TransformComponent, BoundingVolumeComponent, EnabledTag>();
	// 		for (auto& entity : scene->cullResults.visibleEntities) {
	//
	// 			auto newDrawCmd = subMeshEntity_to_drawCommand(scene, reg, entity);
	// 			//const SceneInstancePair mapIndex{ sceneIndex, newDrawCmd.submeshOffset };
	//
	// 			auto it = submeshDrawInstanceData[currentFrame][sceneIndex].find(newDrawCmd.submeshOffset);
	// 			if (it == submeshDrawInstanceData[currentFrame][sceneIndex].end()) {
	// 				drawCmds[currentFrame][sceneIndex].push_back(newDrawCmd);
	// 				auto keyPair = submeshDrawInstanceData[currentFrame][sceneIndex].insert({ newDrawCmd.submeshOffset, BoundedInstanceData(instanceDataArena[currentFrame]) });
	//
	// 				BoundedInstanceData& instanceData = keyPair.first->second;//submeshDrawInstanceData[currentFrame][sceneIndex][keyPair.second];
	// 				instanceData.instances.reserve(8192);
	//
	// 				auto& transComp = grp.get<TransformComponent>(newDrawCmd.subMeshEntity);//scene->registry().get<TransformComponent>(newDrawCmd.subMeshEntity);
	// 				InstanceData instData{};
	// 				instData.matInstanceIndex = newDrawCmd.materialIndex;
	// 				instData.matrixIndex = transComp.dataIndex;
	// 				instanceData.instances.push_back(instData);
	// 				//BoundingVolumeComponent& bVol = grp.get<BoundingVolumeComponent>(newDrawCmd.subMeshEntity);
	// 				//instanceData.bounds.merge(Engine::getInstance()->
	// 				//						  getScene(transComp.sceneIndex)->boundingSystem().cachedLocals()[bVol.coarseIndexLocal]);
	//
	// 			} else {
	// 				BoundedInstanceData& instanceData = it->second;
	// 				auto& transComp = grp.get<TransformComponent>(newDrawCmd.subMeshEntity);//auto& transComp = scene->registry().get<TransformComponent>(newDrawCmd.subMeshEntity);
	// 				InstanceData instData{};
	// 				instData.matInstanceIndex = newDrawCmd.materialIndex;
	// 				instData.matrixIndex = transComp.dataIndex;
	// 				instanceData.instances.push_back(instData);
	// 				//BoundingVolumeComponent& bVol = grp.get<BoundingVolumeComponent>(newDrawCmd.subMeshEntity);
	// 				//instanceData.bounds.merge(Engine::getInstance()->
	// 				//						  getScene(transComp.sceneIndex)->boundingSystem().cachedLocals()[bVol.coarseIndexLocal]);
	//
	// 				submeshKeysWithMultipleInstances[currentFrame][sceneIndex].insert(newDrawCmd.submeshOffset);
	// 			}
	// 		}
	//
	//
	//
	// 		std::sort(drawCmds[currentFrame][sceneIndex].begin(), drawCmds[currentFrame][sceneIndex].end());
	//
	// 	}
	// }

	const auto drwCmdIdx = getDrawCommandBufferIndex();




	// Check the draw commands and issue binds and draw calls
	instanceDataArena[drwCmdIdx]->reset();
	uint32_t drawCount = 0;
	uint32_t pipelinesCount = 0;
	uint32_t setCount = 0;
	bool firstPass = true;
	bool hasDrawn = false;
	size_t stageCursor = 0;
	SceneBase* scene1 = nullptr;
	//uint16_t sceneIndex{};
	//FrameArenaVector<MeshDrawCommand>* cmdScene;
	//std::vector<MeshDrawCommand>* cmdScene;
	//static std::pair<const uint16_t, std::vector<MeshDrawCommand>>* cmdScene[VULKAN_FRAMES_IN_FLIGHT];
	//static robin_hood::pair<uint16_t, std::vector<MeshDrawCommand>>* cmdScene[VULKAN_FRAMES_IN_FLIGHT];


	static std::vector<uint16_t> sceneIndices[VULKAN_FRAMES_IN_FLIGHT];
	sceneIndices[currentFrame].clear();
	static std::vector<Range<size_t>> bufferOffsets[VULKAN_FRAMES_IN_FLIGHT];
	bufferOffsets[currentFrame].clear();
	size_t totalMatBufSize = 0;
	size_t totalInstanceDataBufSize = 0;
	static size_t lastMatBufferSize[VULKAN_FRAMES_IN_FLIGHT] = { 0, 0 };
	static size_t lastInstDataTempSize[VULKAN_FRAMES_IN_FLIGHT] = { 0, 0 };
	ArenaVector<InstanceData> instDataTemp{ ArenaAllocator<InstanceData>{instanceDataArena[drwCmdIdx]} };
	instDataTemp.reserve(8192);
	VkBufferCopy instanceBufferCopy{};
	static std::vector<VkBufferCopy> stagingRegions[VULKAN_FRAMES_IN_FLIGHT];
	//bool stageWritten = false;
	stagingRegions[currentFrame].clear();
	{
		PROFILE_SCOPE("Buffer CPU-Stage");
		for (uint16_t sI = 0; sI < static_cast<uint16_t>(drawCmds[drwCmdIdx].size()); ++sI) {

			if (drawCmds[drwCmdIdx][sI].empty())
				continue;
			scene1 = Engine::getInstance()->getScene(sI);

			sceneIndices[currentFrame].push_back(sI);
			size_t count = sizeof(ModelTransform) * scene1->transformSystem().modelTransforms().size();
			bufferOffsets[currentFrame].push_back(Range<size_t>{totalMatBufSize, count});
			totalMatBufSize += count;


			for (auto& key : submeshKeysWithMultipleInstances[drwCmdIdx][sI]) {

				for (auto& instData : submeshDrawInstanceData[drwCmdIdx][sI][key].instances) {
					instDataTemp.push_back(instData);
				}
			}
		}


		_checkStageRealloc(currentFrame, totalMatBufSize);

		for (size_t s = 0; s < sceneIndices[currentFrame].size(); ++s) {
			auto* thisScene = Engine::getInstance()->getActiveScenes()[sceneIndices[currentFrame][s]];

			std::memcpy(static_cast<uint8_t*>(matStagingPtr[currentFrame]) + stageCursor,
						thisScene->transformSystem().modelTransforms().data(),
						bufferOffsets[currentFrame][s].count);

			stagingRegions[currentFrame].emplace_back(
				VkBufferCopy{ .srcOffset = stageCursor, .dstOffset = bufferOffsets[currentFrame][s].offset, .size = bufferOffsets[currentFrame][s].count });

			stageCursor += bufferOffsets[currentFrame][s].count;
		}
		totalInstanceDataBufSize = sizeof(InstanceData) * instDataTemp.size();
		if (!instDataTemp.empty()) {

			_checkStageRealloc(currentFrame, totalInstanceDataBufSize + totalMatBufSize);
			std::memcpy(static_cast<uint8_t*>(matStagingPtr[currentFrame]) + stageCursor,
						instDataTemp.data(),
						totalInstanceDataBufSize);
			instanceBufferCopy = VkBufferCopy{ .srcOffset = stageCursor, .dstOffset = 0, .size = totalInstanceDataBufSize };
			stageCursor += totalInstanceDataBufSize;

		}
	}








	uint32_t imageIndex = 0;
	{
		PROFILE_SCOPE("WaitFence_AquireNextFrame");
		// Wait for previous frame fence 
		vkWaitForFences(m_vkDevice, 1, &frame.inFlight, VK_TRUE, UINT64_MAX);
		vkResetFences(m_vkDevice, 1, &frame.inFlight);

		// Acquire swapchain image	 		
		auto acquireResult = vkAcquireNextImageKHR(
			m_vkDevice, swapchain,
			UINT64_MAX, frame.imageAvailable,
			VK_NULL_HANDLE, &imageIndex);



		if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || pendingResize) {
			recreateSwapchain();
			return;
		}
	}



	Camera* mainCam = nullptr;
	Frustum* f = nullptr;
	{
		PROFILE_SCOPE("PreSceneDraws");
		// Resize buffers if needed
		if (lastMatBufferSize[currentFrame] < totalMatBufSize) {
			lastMatBufferSize[currentFrame] = totalMatBufSize;

			VkResult vkResult{};
			VkDeviceSize bufferSize = totalMatBufSize;
			VkBufferCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.size = bufferSize;
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
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
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

		if (totalInstanceDataBufSize > lastInstDataTempSize[currentFrame]) {
			lastInstDataTempSize[currentFrame] = totalInstanceDataBufSize;

			VkResult vkResult{};
			VkDeviceSize bufferSize = totalInstanceDataBufSize;
			VkBufferCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.size = bufferSize;

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
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
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



		//Begin command buffer
		vkResetCommandBuffer(frame.cmdBuffer, 0);
		recordCommandBuffer(frame.cmdBuffer, currentFrame);





		// Dispatch light cluster CS
		vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, lightCluster_pipeline);
		vkCmdBindDescriptorSets(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
								lightCluster_pipelineLayout, 0, 3, lightCluster_descSets[currentFrame].data(),
								0, nullptr);

		// Compute proper group counts from clusters and shader local size
		//constexpr uint32_t LOCAL_X = VULKAN_LIGHT_CLUSTER_THREADS_X; // matches [numthreads(�)]
		//constexpr uint32_t LOCAL_Y = VULKAN_LIGHT_CLUSTER_THREADS_Y;
		//constexpr uint32_t LOCAL_Z = VULKAN_LIGHT_CLUSTER_THREADS_Z;
		//auto ceilDiv = [](uint32_t a, uint32_t b) { return (a + b - 1) / b; };

		//const uint32_t groupsX = ceilDiv(camData.clustersX, LOCAL_X);
		//const uint32_t groupsY = ceilDiv(camData.clustersY, LOCAL_Y);
		//const uint32_t groupsZ = ceilDiv(camData.clustersZ, LOCAL_Z);

		vkCmdDispatch(frame.cmdBuffer, VULKAN_LIGHT_CLUSTERS_X, VULKAN_LIGHT_CLUSTERS_Y, VULKAN_LIGHT_CLUSTERS_Z);



		VkViewport viewport{};


		// Set Dynamic state

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

		vkCmdSetDepthTestEnable(frame.cmdBuffer, VK_TRUE);
		vkCmdSetDepthWriteEnable(frame.cmdBuffer, VK_TRUE);
		vkCmdSetDepthCompareOp(frame.cmdBuffer, VK_COMPARE_OP_LESS_OR_EQUAL);


		// Camera
		mainCam = Engine::getInstance()->mainCamera();
		auto& cam_data = mainCam->cameraData();
		cam_data.numLights = static_cast<uint32_t>(lightsData.size());
		cam_data.screenSize[0] = viewport.width;
		cam_data.screenSize[1] = viewport.height;
		cam_data.invScreenSize[0] = 1.0f / cam_data.screenSize[0];
		cam_data.invScreenSize[1] = 1.0f / cam_data.screenSize[1];
		cam_data.clustersX = VULKAN_LIGHT_CLUSTERS_X;
		cam_data.clustersY = VULKAN_LIGHT_CLUSTERS_Y;
		cam_data.clustersZ = VULKAN_LIGHT_CLUSTERS_Z;
		f = &mainCam->getFrustum();

		// Update CameraData cBuffer
		void* mappedData = nullptr;
		vkMapMemory(m_vkDevice, camDataMemory[currentFrame], 0, sizeof(CameraData), 0, &mappedData);
		memcpy(mappedData, &cam_data, sizeof(CameraData));
		vkUnmapMemory(m_vkDevice, camDataMemory[currentFrame]);



		//for (size_t sI = 0; sI < drawCmds[currentFrame].size(); ++sI) //for (auto& cScene : drawCmds[currentFrame])
		//{
		//	PROFILE_SCOPE("BufferCopy");
		//	if (drawCmds[currentFrame][sI].empty())
		//		continue;

		//	cmdScene = &drawCmds[currentFrame][sI];
		//	sceneIndex = sI;
		//	scene = Engine::getInstance()->getScene(sceneIndex);


		//	// upload the data
		//	// Matrices
		//	static size_t lastMatBufferSize[VULKAN_FRAMES_IN_FLIGHT] = { 0, 0 };
		//	{
		//		VkResult vkResult{};
		//		VkDeviceSize bufferSize = sizeof(ModelTransform) * scene->transformSystem().modelTransforms().size();
		//		VkBufferCreateInfo createInfo{};
		//		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		//		createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		//		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		//		createInfo.size = bufferSize;

		//		if (lastMatBufferSize[currentFrame] < bufferSize) {
		//			lastMatBufferSize[currentFrame] = bufferSize;
		//			vkDestroyBuffer(m_vkDevice, matBuf_modelTransforms[currentFrame], nullptr);
		//			modelTransforms_lastBufferSize[currentFrame] = bufferSize;

		//			if (matDevMem_modelTransforms[currentFrame] != VK_NULL_HANDLE)
		//				vkFreeMemory(m_vkDevice, matDevMem_modelTransforms[currentFrame], nullptr);


		//			vkResult = vkCreateBuffer(m_vkDevice, &createInfo, nullptr, &matBuf_modelTransforms[currentFrame]);
		//			VkMemoryRequirements bufferMemReq{};
		//			vkGetBufferMemoryRequirements(m_vkDevice, matBuf_modelTransforms[currentFrame], &bufferMemReq);
		//			VkMemoryAllocateInfo allocInfo{};
		//			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		//			allocInfo.allocationSize = bufferMemReq.size;
		//			allocInfo.memoryTypeIndex = findMemoryType(
		//				bufferMemReq.memoryTypeBits,
		//				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		//				m_phyDevice
		//			);
		//			vkResult = vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &matDevMem_modelTransforms[currentFrame]);
		//			vkResult = vkBindBufferMemory(m_vkDevice, matBuf_modelTransforms[currentFrame], matDevMem_modelTransforms[currentFrame], 0);

		//			VkDescriptorSet matricesSet = bindingToDescriptorSet[{MAT_MODELMATRICES_BIND, MAT_MODELMATRICES_SET, 0}][currentFrame];
		//			std::vector<VkWriteDescriptorSet> descriptorWrites;
		//			VkDescriptorBufferInfo modelMatricesBufferInfo{};
		//			modelMatricesBufferInfo.buffer = matBuf_modelTransforms[currentFrame];
		//			modelMatricesBufferInfo.offset = 0;
		//			modelMatricesBufferInfo.range = VK_WHOLE_SIZE;

		//			VkWriteDescriptorSet matrixDataWrite{};
		//			matrixDataWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//			matrixDataWrite.dstSet = matricesSet;
		//			matrixDataWrite.dstBinding = MAT_MODELMATRICES_BIND;
		//			matrixDataWrite.dstArrayElement = 0;
		//			matrixDataWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		//			matrixDataWrite.descriptorCount = 1;
		//			matrixDataWrite.pBufferInfo = &modelMatricesBufferInfo;
		//			vkUpdateDescriptorSets(m_vkDevice, 1, &matrixDataWrite, 0, nullptr);
		//		}
		//		_checkStageRealloc(bufferSize);
		//		std::memcpy(static_cast<uint8_t*>(matStagingPtr[currentFrame]) + stageCursor,
		//					scene->transformSystem().modelTransforms().data(),
		//					bufferSize);
		//		VkBufferCopy c{ .srcOffset = stageCursor, .dstOffset = 0, .size = bufferSize };
		//		vkCmdCopyBuffer(frame.cmdBuffer, matStagingBuf[currentFrame], matBuf_modelTransforms[currentFrame], 1, &c);
		//		stageCursor += bufferSize;

		//	}
		//	// instanceData on gpu
		//	ArenaVector<InstanceData> instDataTemp{ ArenaAllocator<InstanceData>{instanceDataArena[currentFrame]} };
		//	instDataTemp.reserve(8192);
		//	for (auto& key : submeshKeysWithMultipleInstances[currentFrame][sceneIndex]) {
		//		const SceneInstancePair pair{ sceneIndex, key };

		//		for (auto& instData : submeshDrawInstanceData[currentFrame][pair].instances) {
		//			instDataTemp.push_back(instData);
		//		}

		//	}

		//	// Copy instance data to GPU
		//	
		//	if (!instDataTemp.empty()) {
		//		VkResult vkResult{};
		//		VkDeviceSize bufferSize = sizeof(InstanceData) * instDataTemp.size();
		//		VkBufferCreateInfo createInfo{};
		//		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		//		createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		//		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		//		createInfo.size = bufferSize;
		//		if (bufferSize > lastInstDataTempSize[currentFrame]) {
		//			lastInstDataTempSize[currentFrame] = bufferSize;

		//			vkDestroyBuffer(m_vkDevice, instanceIndexBuffer[currentFrame], nullptr);
		//			instancesIndex_lastBufferSize[currentFrame] = bufferSize;

		//			if (instanceIndexMemory[currentFrame] != VK_NULL_HANDLE)
		//				vkFreeMemory(m_vkDevice, instanceIndexMemory[currentFrame], nullptr);

		//			vkResult = vkCreateBuffer(m_vkDevice, &createInfo, nullptr, &instanceIndexBuffer[currentFrame]);
		//			VkMemoryRequirements bufferMemReq{};
		//			vkGetBufferMemoryRequirements(m_vkDevice, instanceIndexBuffer[currentFrame], &bufferMemReq);
		//			VkMemoryAllocateInfo allocInfo{};
		//			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		//			allocInfo.allocationSize = bufferMemReq.size;
		//			allocInfo.memoryTypeIndex = findMemoryType(
		//				bufferMemReq.memoryTypeBits,
		//				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		//				m_phyDevice
		//			);
		//			vkResult = vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &instanceIndexMemory[currentFrame]);
		//			vkResult = vkBindBufferMemory(m_vkDevice, instanceIndexBuffer[currentFrame], instanceIndexMemory[currentFrame], 0);

		//			VkDescriptorSet instanceIndexSet = bindingToDescriptorSet[{MAT_BASE_INSTANCES_DATA_BIND, MAT_BASE_INSTANCES_DATA_SET, 0}][currentFrame];
		//			std::vector<VkWriteDescriptorSet> descriptorWrites;
		//			VkDescriptorBufferInfo instanceIndexBufferInfo{};
		//			instanceIndexBufferInfo.buffer = instanceIndexBuffer[currentFrame];
		//			instanceIndexBufferInfo.offset = 0;
		//			instanceIndexBufferInfo.range = VK_WHOLE_SIZE;
		//			VkWriteDescriptorSet instanceDataWrite{};
		//			instanceDataWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//			instanceDataWrite.dstSet = instanceIndexSet;
		//			instanceDataWrite.dstBinding = MAT_BASE_INSTANCES_DATA_BIND;
		//			instanceDataWrite.dstArrayElement = 0;
		//			instanceDataWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		//			instanceDataWrite.descriptorCount = 1;
		//			instanceDataWrite.pBufferInfo = &instanceIndexBufferInfo;
		//			vkUpdateDescriptorSets(m_vkDevice, 1, &instanceDataWrite, 0, nullptr);
		//		}
		//		_checkStageRealloc(bufferSize);
		//		std::memcpy(static_cast<uint8_t*>(matStagingPtr[currentFrame]) + stageCursor,
		//					instDataTemp.data(),
		//					bufferSize);
		//		VkBufferCopy c{ .srcOffset = stageCursor, .dstOffset = 0, .size = bufferSize };
		//		vkCmdCopyBuffer(frame.cmdBuffer, matStagingBuf[currentFrame], instanceIndexBuffer[currentFrame], 1, &c);
		//		stageCursor += bufferSize;

		//	}




		if (totalMatBufSize > 0)
			vkCmdCopyBuffer(frame.cmdBuffer, matStagingBuf[currentFrame], matBuf_modelTransforms[currentFrame],
							static_cast<uint32_t>(stagingRegions[currentFrame].size()), stagingRegions[currentFrame].data());

		if (totalInstanceDataBufSize > 0)
			vkCmdCopyBuffer(frame.cmdBuffer, matStagingBuf[currentFrame], instanceIndexBuffer[currentFrame], 1, &instanceBufferCopy);



		// Barrier
#if VK_HEADER_VERSION >= 230  // assuming 1.3 / sync2
		//VkMemoryBarrier2 memBarrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER_2 };
		//memBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		//memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT; // writes in CS
		//memBarrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		//memBarrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT;

		//VkDependencyInfo dep{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		//dep.memoryBarrierCount = 1;
		//dep.pMemoryBarriers = &memBarrier;

		VkBufferMemoryBarrier2 bufferBarriers[] =
		{
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
				.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
				.buffer = lightsClusterIndicesBuffer[currentFrame],
				.offset = 0,
				.size = VK_WHOLE_SIZE
			},
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
				.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
				.buffer = lightsClusterCountBuffer[currentFrame],
				.offset = 0,
				.size = VK_WHOLE_SIZE
			}
		};

		VkDependencyInfo dep{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		dep.bufferMemoryBarrierCount = 2;
		dep.pBufferMemoryBarriers = bufferBarriers;



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


	}


		{
			PROFILE_SCOPE("SceneDraws");

			// bind vertex buffer
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(frame.cmdBuffer, 0, 1, &vertexBuffer, offsets);
			vkCmdBindIndexBuffer(frame.cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			// Begin Scene Render Pass
			if (firstPass) {
				VkClearValue clearValues[2] = {};
				clearValues[0].color = { ctx.clearColor[0], ctx.clearColor[1], ctx.clearColor[2], ctx.clearColor[3] };
				clearValues[1].depthStencil = { 1.0f, 0 };

				VkRenderPassBeginInfo renderPassInfo{};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassInfo.renderPass = rendPasses[0];
				renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
				renderPassInfo.renderArea.offset = { 0, 0 };
				renderPassInfo.renderArea.extent = swapchainExtent;
				renderPassInfo.clearValueCount = 2;
				renderPassInfo.pClearValues = clearValues;
				vkCmdBeginRenderPass(frame.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				firstPass = false;
				hasDrawn = true;
			} else {
				VkRenderPassBeginInfo renderPassInfo{};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassInfo.renderPass = rendPasses[0];
				renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
				renderPassInfo.renderArea.offset = { 0, 0 };
				renderPassInfo.renderArea.extent = swapchainExtent;
				renderPassInfo.clearValueCount = 0;
				renderPassInfo.pClearValues = nullptr;
				vkCmdBeginRenderPass(frame.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			}

			uint32_t lastPipelineIndex = UINT32_INVALID - 1;
			//Material* lastMaterial = nullptr;
			//uint32_t lastDescCount = 0;
			uint32_t lastMatIndex = UINT32_INVALID;
			VkDescriptorSet lastSets[4]{ nullptr, nullptr, nullptr, nullptr };
			uint32_t firstSet = 0;
			bool pendingRebind = false;

			for (size_t i = 0; i < sceneIndices[currentFrame].size(); ++i) {			//for (auto& cmd : *cmdScene) {
				size_t sceneIndex = sceneIndices[currentFrame][i];
				auto& cmdList = drawCmds[drwCmdIdx][sceneIndex];
				if (cmdList.empty())
					continue;

				auto grp = Engine::getInstance()->getActiveScenes()[sceneIndex]->registry().group<TransformComponent, BoundingVolumeComponent, EnabledTag>();
				for (auto& cmd : cmdList) {
					auto& instData = submeshDrawInstanceData[drwCmdIdx][sceneIndex][cmd.submeshOffset];
					auto instanceSize = instData.instances.size();
					//if (instanceSize > 1) {
					//	auto& aabb = instData.bounds;
					//	if (!MMath::aabbVisible(aabb.min, aabb.max, *f))
					//		continue;
					//}



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
									setCount = 0;
									pendingRebind = true;
								}
								lastSets[combo.set] = descSet[currentFrame];
								setCount++;
							}
						}

						if (pendingRebind) {
							for (size_t fSeti = 0; fSeti  < 4; ++fSeti ) {
								if (lastSets[fSeti] != nullptr) {
									firstSet = fSeti;
									break;
								}
							}
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
						push.flags = static_cast<uint32_t>(BaseMatPushFlags::Instanced);
						vkCmdPushConstants(frame.cmdBuffer, pipelineLayouts[matBase->pipelineLayoutId],
							VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
										   0, sizeof(BaseMatPush), &push);

						auto& submesh = scene1->sceneRender().getSubMeshes()[cmd.submeshOffset];
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
						auto& transComp = grp.get<TransformComponent>(cmd.subMeshEntity);//scene->registry().get<TransformComponent>(cmd.subMeshEntity);
						push.modelToWorld = scene1->transformSystem().modelTransforms()[transComp.dataIndex].mtw;
						vkCmdPushConstants(frame.cmdBuffer, pipelineLayouts[matBase->pipelineLayoutId], VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
										   0, sizeof(BaseMatPush), &push);


						auto& submesh = scene1->sceneRender().getSubMeshes()[cmd.submeshOffset];
						auto vertexOffset = submesh.vertexOffset;
						auto indexOffset = submesh.indexOffset;
						auto indexCount = submesh.indexCount;

						vkCmdDrawIndexed(frame.cmdBuffer, indexCount, 1, indexOffset, vertexOffset, 0);
						drawCount++;
					}
				}				
			}

			// End Render Pass
			vkCmdEndRenderPass(frame.cmdBuffer);
		}
	
	
	{
		PROFILE_SCOPE("DebugDraws");

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
											& shapeRendererDescSet[currentFrame],
											0, nullptr);
											pipelinesCount++;
											setCount++;
											anyAabbsDrawn = true;

											vkCmdSetDepthTestEnable(frame.cmdBuffer, VK_FALSE);
											vkCmdSetDepthWriteEnable(frame.cmdBuffer, VK_FALSE);
											//vkCmdSetDepthCompareOp(frame.cmdBuffer, VK_COMPARE_OP_LESS_OR_EQUAL);


											VkClearValue clearValues[2] = {};
											clearValues[0].color = { ctx.clearColor[0], ctx.clearColor[1], ctx.clearColor[2], 0.0f };
											clearValues[1].depthStencil = { 1.0f, 0 };
											VkRenderPassBeginInfo renderPassInfo{};
											renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
											renderPassInfo.renderPass = rendPasses[0];
											renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
											renderPassInfo.renderArea.offset = { 0, 0 };
											renderPassInfo.renderArea.extent = swapchainExtent;
											renderPassInfo.clearValueCount = 2;
											renderPassInfo.pClearValues = clearValues;
											vkCmdBeginRenderPass(frame.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
											hasDrawn = true;
			}

			if (scene->sceneRender().drawOccluders()) {
				for (auto& entity : scene->cullResults.activeOccluders) {
					auto [bound, transform] = scene->registry().get<BoundingVolumeComponent, TransformComponent>(entity);
					AABB worldBox = scene->boundingSystem().aabbs()[bound.coarseIndexWorld];
					ShapePush shapePush{};
					shapePush.modelToWorld = Transform::composeTRS(&scene->registry(), entity);
					shapePush.color = { 1.0f, 0.01f, 0.02f, 0.1f };
					shapePush.rotation = glm::quat();
					shapePush.aabb = { worldBox.min, worldBox.max };
					shapePush.drawNumber = shapeDraws++;

					vkCmdPushConstants(frame.cmdBuffer, pipelineLayouts[shapeMat->pipelineLayoutId], VK_SHADER_STAGE_VERTEX_BIT,
									   0, sizeof(ShapePush), &shapePush);

					vkCmdDrawIndexed(frame.cmdBuffer, 36, 1, 0, 0, 0);
					drawCount++;
				}
			} else if (scene->sceneRender().drawNodes()) {

				float largestNodeVol = 0;
				for (size_t n = 0; n < scene->bvhSystem().bvh().getNodeCount(); ++n) {
					auto& node = scene->bvhSystem().bvh().getCurrentNodes()[n];
					AABB box = node.bounds;

					if (n == 0) {
						largestNodeVol = box.volume();
					}
					float nodeVol = box.volume();
					float alpha = std::clamp(largestNodeVol / nodeVol * 0.005f, 0.0f, 0.9f);

					ShapePush shapePush{};
					shapePush.modelToWorld = glm::mat4{ 1 };
					shapePush.color = { 1.0f, 0.01f, 1.0f, alpha };
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

					shapePush.modelToWorld = glm::mat4{ 1 };
					shapePush.color = { 0.02f, 1.0f, 0.02f, 0.1f };
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
		// End Render Pass for debugs
		if (anyAabbsDrawn)
			vkCmdEndRenderPass(frame.cmdBuffer);

	}

	{
		PROFILE_SCOPE("DrawPresentation");

		// Just clear screen if nothing has been rendered
		if (!hasDrawn) {
			VkClearValue clearValues[2] = {};
			clearValues[0].color = { ctx.clearColor[0], ctx.clearColor[1], ctx.clearColor[2], ctx.clearColor[3] };
			clearValues[1].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = rendPasses[0];
			renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapchainExtent;
			renderPassInfo.clearValueCount = 2;
			renderPassInfo.pClearValues = clearValues;
			vkCmdBeginRenderPass(frame.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdEndRenderPass(frame.cmdBuffer);
		}


		// End command buffer
		if (vkEndCommandBuffer(frame.cmdBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

		m_lastDrawcallCount = drawCount;
		m_lastPipelineSwitches = pipelinesCount;


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
