#include "BVH.hpp"
#include "BoundingVolume.hpp"
#include "Transform.hpp"
#include "Frustum.hpp"

void DualBVH::_recursiveWorker(DualBVH* _this, uint8_t threadId) {

    while (true) {

        _this->startSemas[threadId]->acquire();
        if (!_this->workersRunning)
            return;

        const WorkerPkg& pkg = _this->workerPkgs[threadId].load(std::memory_order_acquire);
        auto result = _this->buildRecursive(*pkg.primitiveIds, pkg.depth, pkg.parent, pkg.index);
        _this->workerResults[threadId].store(result, std::memory_order_release);
        _this->doneSemas[threadId]->release();
    }
}

uint8_t DualBVH::_getIndexToUseThisFrame(uint8_t lastFrameIndex) {

    //uint8_t result;
    //while (result = _threadIndexToUse.load(std::memory_order_acquire) == UINT8_INVALID) {}
    //
    //return result;

    //if (_threadLastSuccesful.load())
    //    return lastFrameIndex;
    //while (_threadIndexToUse == UINT8_INVALID) {}

    return _threadIndexToUse.load(std::memory_order_acquire);
}

void DualBVH::_buildThreadMethod(DualBVH* _this, ArenaRegistry& registry) {

    while (true) {


        _this->_threadStart.acquire();
        if (!_this->_threadRunning) {
            return;
        }
        
        uint8_t index;
        uint8_t readIndex = _this->_threadIndexToUse.load(std::memory_order_acquire);
        if (readIndex == UINT8_INVALID)
            index = 0;
        else {
            index = (readIndex + 1) % 2;
        }

        auto view = registry.view<BoundingVolumeComponent>();
        for (auto entity : view) {

            
            auto it = _this->entityToPrimitive[index].find(entity);
            if (it == _this->entityToPrimitive[index].end()) {

                _this->rebuildPrimitives(registry, index);
                break;
            }
        }
        _this->buildNodes(registry, index);

        _this->_threadIndexToUse.store(index, std::memory_order_release);
        _this->_threadDone.release();
    }
}

void DualBVH::rebuildPrimitives(ArenaRegistry& registry, uint8_t index) {
    // Clear existing data
    primitives[index].clear();
    //primitiveIndices[index].clear();
    entityToPrimitive[index].clear();
    alwaysVisible[index].clear();

    // Collect all entities with Transform and AABB components
    auto view = registry.view<BoundingVolumeComponent>();
    uint32_t primIndex = 0;
    for (auto [entity, boundComp] : view.each()) {

        if (boundComp.flags & static_cast<uint32_t>(BoundingVolumeFlags::Occluder) || 
            boundComp.flags & static_cast<uint32_t>(BoundingVolumeFlags::Occludee)) {

            BoundingVolume bounds = BoundingVolume{ &registry, entity };
            primitives[index].emplace_back(entity, bounds);

            if (boundComp.flags & static_cast<uint32_t>(BoundingVolumeFlags::Occluder)) {
                auto cornerIndex = occluderCorners.size();
                auto verts = bounds.getCoarseAABB().getVertices();
                occluderCorners[index].insert(occluderCorners[index].end(), verts.begin(), verts.end());
                occluderIndices[index].push_back({ primIndex, cornerIndex, 0.0f });
            }

            entityToPrimitive[index][entity] = primIndex++;
        } else {
            alwaysVisible[index].push_back(entity);
        }


    }

    if (primitives.empty()) {
        nodeCount[index] = 0;
        rootIndex[index] = 0;
        return;
    }

    // Build primitive indices array
    //primitiveIndices[index].resize(primitives[index].size());
    //std::iota(primitiveIndices[index].begin(), primitiveIndices[index].end(), 0);
}

uint32_t DualBVH::buildRecursive(std::vector<uint32_t>& primitiveIds, uint32_t depth, uint32_t parent, uint8_t index) {
     if (primitiveIds.empty()) return 0;


     uint32_t nodeIndex = nodeCount[index].fetch_add(1, std::memory_order_acquire);
     //if (nodeIndex >= nodes[index].size()) {

     //    nodes[index].resize(nodeIndex + 1);

     //}

     BVHNode& node = nodes[index][nodeIndex];


     node.parentIndex = parent;
     node.bounds = computeBounds(primitiveIds, index);
     node.setDirty(false);

     // Leaf node condition
     if (primitiveIds.size() <= settings.maxLeafPrimitives || depth > settings.maxDepth) {
         node.setLeaf(true);

         // find lowest
         //uint32_t lowestId = 0xffffffff;
         //for (auto& index : primitiveIds) {
         //    if (index < lowestId)
         //        lowestId = index;
         //}

         //node.firstPrimitive = indicesCount[index].fetch_add(primitiveIds.size(), std::memory_order_acquire);//primitiveIndices[index].size();
         node.primCount = primitiveIds.size();

         for (size_t i = 0; i < primitiveIds.size(); ++i) {
             //primitiveIndices[index][node.firstPrimitive + i] = primitiveIds[i];
             node.primIndices[i] = primitiveIds[i];
         }

         //for (uint32_t primId : primitiveIds) {
         //    primitiveIndices[index][node.firstPrimitive] = primId;//push_back(primId);
         //}

         return nodeIndex;
     }
     // Internal node - find best split
     int bestAxis;
     float bestPos;
     uint32_t bestCost = findBestSplit(primitiveIds, bestAxis, bestPos, index);

     // Partition primitives
     auto partition = std::partition(primitiveIds.begin(), primitiveIds.end(),
                                     [&](uint32_t primId) {
                                         return primitives[index][primId].bounds.getCoarseAABB().center()[bestAxis] < bestPos;
                                     });

     std::vector<uint32_t> leftPrims(primitiveIds.begin(), partition);
     std::vector<uint32_t> rightPrims(partition, primitiveIds.end());

     // Ensure both sides have primitives
     if (leftPrims.empty() || rightPrims.empty()) {
         // Fallback to median split
         std::sort(primitiveIds.begin(), primitiveIds.end(),
                   [&](uint32_t a, uint32_t b) {
                       return primitives[index][a].bounds.getCoarseAABB().center()[bestAxis] <
                           primitives[index][b].bounds.getCoarseAABB().center()[bestAxis];
                   });

         size_t mid = primitiveIds.size() / 2;
         leftPrims.assign(primitiveIds.begin(), primitiveIds.begin() + mid);
         rightPrims.assign(primitiveIds.begin() + mid, primitiveIds.end());
     }

     node.setLeaf(false);
     node.splitAxis = bestAxis;
     if (depth < 2) {

         auto idL = _getNextThreadId();
         workerPrimsTemp[idL].resize(leftPrims.size());
         std::memcpy(workerPrimsTemp[idL].data(), leftPrims.data(), leftPrims.size() * sizeof(uint32_t));
         WorkerPkg l_pkg;
         l_pkg.primitiveIds = &workerPrimsTemp[idL];
         l_pkg.depth = depth + 1;
         l_pkg.index = index;
         l_pkg.parent = nodeIndex;
         workerPkgs[idL].store(l_pkg, std::memory_order_release);
         startSemas[idL]->release();

         auto idR = _getNextThreadId();
         workerPrimsTemp[idR].resize(rightPrims.size());
         std::memcpy(workerPrimsTemp[idR].data(), rightPrims.data(), rightPrims.size() * sizeof(uint32_t));
         WorkerPkg r_pkg;
         r_pkg.primitiveIds = &workerPrimsTemp[idR];
         r_pkg.depth = depth + 1;
         r_pkg.index = index;
         r_pkg.parent = nodeIndex;
         workerPkgs[idR].store(r_pkg, std::memory_order_release);
         startSemas[idR]->release();

         doneSemas[idL]->acquire();
         doneSemas[idR]->acquire();
         node.leftChild = workerResults[idL].load(std::memory_order_acquire);
         node.rightChild = workerResults[idR].load(std::memory_order_acquire);


     } else {
         node.leftChild = buildRecursive(leftPrims, depth + 1, nodeIndex, index);
         node.rightChild = buildRecursive(rightPrims, depth + 1, nodeIndex, index);
     }

     return nodeIndex;
    }

uint32_t DualBVH::findBestSplit(const std::vector<uint32_t>& primitiveIds, int& bestAxis, float& bestPos, uint8_t index) {

    if (settings.useMedianSplit) {
        // Simple median split
        AABB bounds = computeBounds(primitiveIds, index);
        glm::vec3 extent = bounds.size();

        bestAxis = 0;
        if (extent.y > extent.x) bestAxis = 1;
        if (extent.z > extent[bestAxis]) bestAxis = 2;

        bestPos = bounds.center()[bestAxis];
        return 0; // No cost calculation for median split
    }

    // Surface Area Heuristic (SAH)
    uint32_t bestCost = UINT32_MAX;
    bestAxis = 0;
    bestPos = 0.0f;

    AABB totalBounds = computeBounds(primitiveIds, index);
    float totalSA = totalBounds.surfaceArea();

    for (int axis = 0; axis < 3; ++axis) {
        // Sort primitives along this axis
        std::vector<uint32_t> sorted = primitiveIds;
        std::sort(sorted.begin(), sorted.end(),
                  [&](uint32_t a, uint32_t b) {
                      return primitives[index][a].bounds.getCoarseAABB().center()[axis] <
                          primitives[index][b].bounds.getCoarseAABB().center()[axis];
                  });

        // Try different split positions
        for (size_t i = 1; i < sorted.size(); ++i) {
            std::vector<uint32_t> leftPrims(sorted.begin(), sorted.begin() + i);
            std::vector<uint32_t> rightPrims(sorted.begin() + i, sorted.end());

            AABB leftBounds = computeBounds(leftPrims, index);
            AABB rightBounds = computeBounds(rightPrims, index);

            float leftSA = leftBounds.surfaceArea();
            float rightSA = rightBounds.surfaceArea();

            // SAH cost = traversal_cost + P(left) * cost(left) + P(right) * cost(right)
            uint32_t cost = static_cast<uint32_t>(
                1.0f + (leftSA / totalSA) * leftPrims.size() +
                (rightSA / totalSA) * rightPrims.size()
                );

            if (cost < bestCost) {
                bestCost = cost;
                bestAxis = axis;
                bestPos = (primitives[index][sorted[i - 1]].bounds.getCoarseAABB().center()[axis] +
                           primitives[index][sorted[i]].bounds.getCoarseAABB().center()[axis]) * 0.5f;
            }
        }
    }

    return bestCost;
}

void DualBVH::collectOccluders(const glm::vec3& cameraPos, const Frustum& frustum, std::vector<OccluderData>& occluders) const {

    //std::vector<uint32_t> nodeStack;
    //nodeStack.reserve(64);
    //nodeStack.push_back(rootIndex);

    //while (!nodeStack.empty()) {
    //    uint32_t nodeIndex = nodeStack.back();
    //    nodeStack.pop_back();

    //    const BVHNode& node = nodes[nodeIndex];

    //    if (!MMath::aabbVisible(node.bounds.min, node.bounds.max, frustum)) {
    //        continue;
    //    }

    //    if (node.isLeaf()) {
    //        for (uint32_t i = 0; i < node.primitiveCount; ++i) {

    //            // ignore if cam is inside
    //            if (node.bounds.contains(cameraPos))
    //                continue;

    //            uint32_t primIndex = primitiveIndices[node.firstPrimitive + i];
    //            const BVHPrimitive& prim = primitives[primIndex];

    //            auto verts = prim.worldBounds.getVertices();
    //            auto closest = FLT_MAX;
    //            for (auto& vert : verts) {
    //                auto sqrLen = glm::length2(vert - cameraPos);
    //                if (sqrLen < closest)
    //                    closest = sqrLen;
    //            }

    //            auto center = prim.worldBounds.center();
    //            float depth = closest;//glm::length(center - cameraPos);
    //            glm::vec3 size = prim.worldBounds.size();

    //            // Heuristic: large objects close to camera are potential occluders
    //            float volume = size.x * size.y * size.z;
    //            bool isOccluder = (volume > 1.0f && depth < 50.0f) || volume > 10.0f;

    //            occluders.emplace_back(prim.entity, prim.worldBounds, depth, isOccluder);
    //        }
    //    } else {
    //        if (node.leftChild != 0) nodeStack.push_back(node.leftChild);
    //        if (node.rightChild != 0) nodeStack.push_back(node.rightChild);
    //    }
    //}
}



void DualBVH::buildNodes(ArenaRegistry& registry, uint8_t index) {
    // Clear existing data
    //nodes[index].clear();
    //primitives[index].clear();
    primitiveIndices[index].clear();
    //entityToPrimitive[index].clear();
    occluderIndices[index].clear();
    occluderCorners[index].clear();
    //primitiveIndices[index].clear();
    //// Collect all entities with Transform and AABB components
    //auto view = registry.view<BoundingVolumeComponent>();
    //uint32_t primIndex = 0;
    //for (auto [entity, boundComp] : view.each()) {

    //    BoundingVolume bounds = BoundingVolume{ &registry, entity };
    //    primitives[index].emplace_back(entity, bounds);
    //    primitives[index].back().frameUpdated = currentFrame;

    //    if (boundComp.flags & static_cast<uint32_t>(BoundingVolumeFlags::Occluder)) {
    //        auto cornerIndex = occluderCorners.size();
    //        auto verts = bounds.getCoarseAABB().getVertices();
    //        occluderCorners[index].insert(occluderCorners[index].end(), verts.begin(), verts.end());
    //        occluderIndices[index].push_back({ primIndex, cornerIndex, 0.0f });
    //    }

    //    entityToPrimitive[index][entity] = primIndex++;
    //}

    //if (primitives.empty()) {
    //    nodeCount[index] = 0;
    //    rootIndex[index] = 0;
    //    return;
    //}

    //// Build primitive indices array
    //primitiveIndices[index].resize(primitives.size());
    //std::iota(primitiveIndices[index].begin(), primitiveIndices[index].end(), 0);

    // Build tree
    std::vector<uint32_t> allPrimitives(primitives[index].size());
    std::iota(allPrimitives.begin(), allPrimitives.end(), 0);

    primitiveIndices[index].resize(primitives[index].size());
    nodes[index].resize(primitives[index].size() * 2);  // Worst case
    nodeCount[index] = 0;
    indicesCount[index] = 0;
    rootIndex[index] = buildRecursive(allPrimitives, 0, 0, index);

    //dirtyCount = 0;
}

void DualBVH::incrementalUpdate(ArenaRegistry& registry, uint8_t index) {


    if (primitives[index].empty()) {
        rebuildPrimitives(registry, index);
        buildNodes(registry, index);
        return;
    }

    // Update all dirty primitives
    auto view = registry.view<BoundingVolumeComponent>();
    uint32_t updatedCount = 0;

    for (auto entity : view) {
        auto it = entityToPrimitive[index].find(entity);
        if (it == entityToPrimitive[index].end()) {
            // New entity - trigger rebuild
            buildNodes(registry, index);
            return;
        }

        //uint32_t primIndex = it->second;
        //if (primitives[primIndex].frameUpdated < currentFrame) {
        //    TransformComponent& transComp = registry.get<TransformComponent>(entity);
        //    if (!transComp.state.hasFlag(ObjectState::MovedThisFrame))
        //        continue;
        //    Transform transform = Transform{ &registry, entity };
        //    updatePrimitive(primIndex, transform.localToWorld());
        //    updatedCount++;
        //}
    }

    // Check if rebuild is needed
    //if (needsRebuild()) {
    //    rebuild(registry, index);
    //}
}




//void DualBVH::frustumCull(DualBVH::TraversalResult& result, const Frustum& f) const {
//    //TraversalResult result;
//        if (isEmpty()) return; //result;
//
//    std::vector<uint32_t> nodeStack;
//    nodeStack.reserve(64);
//    nodeStack.push_back(rootIndex);
//
//    while (!nodeStack.empty()) {
//        uint32_t nodeIndex = nodeStack.back();
//        nodeStack.pop_back();
//
//        const BVHNode& node = nodes[nodeIndex];
//        result.nodesVisited++;
//
//        // Test AABB against frustum
//        if (!MMath::aabbVisible(node.bounds.min, node.bounds.max, f)) {
//            continue;
//        }
//
//        if (node.isLeaf()) {
//            // Add all primitives in this leaf
//            for (uint32_t i = 0; i < node.primitiveCount; ++i) {
//                uint32_t primIndex = primitiveIndices[node.firstPrimitive + i];
//                result.visibleEntities.push_back(primitives[primIndex].entity);
//                result.primitivesVisited++;
//            }
//        } else {
//            // Add children to stack (back-to-front for better culling)
//            if (node.rightChild != 0) nodeStack.push_back(node.rightChild);
//            if (node.leftChild != 0) nodeStack.push_back(node.leftChild);
//        }
//    }
//
//    return; //result;
//}

void DualBVH::frustumCullWithOcclusion(
    DualBVH::TraversalResult& result,
    const Frustum& frustum,
    const glm::vec3& cameraPos,
    ArenaRegistry* const registry,
    OcclusionMethod method) {

    curFrameIndex = _getIndexToUseThisFrame(curFrameIndex);
    if (curFrameIndex == UINT8_INVALID) {
        return;
    }
    auto frameIndex = curFrameIndex;

    for (auto& entity : alwaysVisible[curFrameIndex])
        result.visibleEntities.push_back(entity);

    if (isEmpty(frameIndex))
        return;
    

    // Collect potential occluders first (front-to-back)
    std::vector<OccluderData> potentialOccluders;
    for (auto& [index, offset, depth] : occluderIndices[frameIndex]) {

        auto& prim = primitives[frameIndex][index];
        AABB worldBounds = prim.bounds.getCoarseAABB();
        if (!MMath::aabbVisible(worldBounds.min, worldBounds.max, frustum)) {
            continue;
        }
        float d[8] = {
             glm::length2(occluderCorners[frameIndex][offset + 0] - cameraPos),
             glm::length2(occluderCorners[frameIndex][offset + 1] - cameraPos),
             glm::length2(occluderCorners[frameIndex][offset + 2] - cameraPos),
             glm::length2(occluderCorners[frameIndex][offset + 3] - cameraPos),
             glm::length2(occluderCorners[frameIndex][offset + 4] - cameraPos),
             glm::length2(occluderCorners[frameIndex][offset + 5] - cameraPos),
             glm::length2(occluderCorners[frameIndex][offset + 6] - cameraPos),
             glm::length2(occluderCorners[frameIndex][offset + 7] - cameraPos),
        };
        __m256 v = _mm256_loadu_ps(d);
        float mn;
        MMath::hmin8(v, mn);
        float closest = mn;
        //auto closest = FLT_MAX;
        //for (auto& vert : verts) {
        //    auto sqrLen = glm::length2(vert - cameraPos);
        //    if (sqrLen < closest)
        //        closest = sqrLen;
        //}

        potentialOccluders.push_back({prim.entity, closest, offset, index});
    }
    //collectOccluders(cameraPos, frustum, potentialOccluders);
    //for (const auto& occluder : potentialOccluders) {
    //    result.activeOccluders.push_back(occluder.entity);
    //}

    // Sort occluders front-to-back
    std::sort(potentialOccluders.begin(), potentialOccluders.end(),
              [](const OccluderData& a, const OccluderData& b) {
                  return a.depth < b.depth;
                  //return a.volume > b.volume;
              });

    // Build active occluder set (simplified - you'd want more sophisticated occlusion volumes)
    std::vector<OccIndexCornerIndexDepthTuple> activeOccluders;
    for (const auto& occluder : potentialOccluders) {                           //////////////////// TODO TODO TODO TODO!!!
            activeOccluders.push_back({ occluder.primIndex, occluder.cornersOffset, occluder.depth });
            result.activeOccluders.push_back(occluder.entity);
            // Limit number of active occluders for performance
            if (result.activeOccluders.size() >= 8) break;
        
    }

    // Traverse with occlusion
    //std::vector<TraversalNode> nodeStack;
    //nodeStack.reserve(64);
    nodeStack[frameIndex].clear();

    float rootMinDepth, rootMaxDepth;
    computeNodeDepthRange(rootIndex[frameIndex], cameraPos, rootMinDepth, rootMaxDepth, frameIndex);
    nodeStack[frameIndex].push_back({ rootIndex[frameIndex], rootMinDepth, rootMaxDepth });

    while (!nodeStack[frameIndex].empty()) {
        TraversalNode current = nodeStack[frameIndex].back();
        nodeStack[frameIndex].pop_back();

        const BVHNode& node = nodes[frameIndex][current.nodeIndex];
        result.nodesVisited++;

        // Frustum test first (cheapest)
        if (!MMath::aabbVisible(node.bounds.min, node.bounds.max, frustum)) {
            result.nodesCulledByFrustum++;
            continue;
        }

        //// Occlusion test
        //if (method != OcclusionMethod::NONE && !activeOccluders.empty()) {
        //    if (!node.bounds.contains(cameraPos)) {
        //        auto corners = node.bounds.getVertices();
        //        float d[8] = {
        //            glm::length2(corners[0] - cameraPos),
        //            glm::length2(corners[1] - cameraPos),
        //            glm::length2(corners[2] - cameraPos),
        //            glm::length2(corners[3] - cameraPos),
        //            glm::length2(corners[4] - cameraPos),
        //            glm::length2(corners[5] - cameraPos),
        //            glm::length2(corners[6] - cameraPos),
        //            glm::length2(corners[7] - cameraPos),
        //        };
        //        __m256 v = _mm256_loadu_ps(d);
        //        float mn;
        //        MMath::hmin8(v, mn);
        //        if (isOccludedRaycast(node.bounds, activeOccluders, cameraPos, mn)) {
        //            result.nodesCulledByOcclusion++;
        //            continue;
        //        }
        //        
        //    }
        //}

        if (node.isLeaf()) {
            // Process leaf primitives
            for (uint32_t i = 0; i < node.primCount; ++i) {
                uint32_t primIndex = node.primIndices[i];//primitiveIndices[frameIndex][node.firstPrimitive + i];
                const BVHPrimitive& prim = primitives[frameIndex][primIndex];

                // Fine-grained occlusion test for individual primitives
                bool occluded = false;
                AABB primWorldBounds = prim.bounds.getCoarseAABB();
                if (!MMath::aabbVisible(primWorldBounds.min, primWorldBounds.max, frustum)) {
                    result.nodesCulledByFrustum++;
                    continue;
                }

                if (method != OcclusionMethod::NONE && !activeOccluders.empty()) {
                    float closest = 0.0f;
                    BoundingVolume primBounds{ registry, prim.entity };
                    auto primVerts = primBounds.getCoarseAABB().getVertices();
                    float d[8] = {
                        glm::length2(primVerts[0] - cameraPos),
                        glm::length2(primVerts[1] - cameraPos),
                        glm::length2(primVerts[2] - cameraPos),
                        glm::length2(primVerts[3] - cameraPos),
                        glm::length2(primVerts[4] - cameraPos),
                        glm::length2(primVerts[5] - cameraPos),
                        glm::length2(primVerts[6] - cameraPos),
                        glm::length2(primVerts[7] - cameraPos),
                    };
                    __m256 v = _mm256_loadu_ps(d);
                    float mn;
                    MMath::hmin8(v, mn);

                    occluded = isOccludedRaycast(primWorldBounds, activeOccluders, cameraPos, mn, frameIndex) &&
                        !primWorldBounds.contains(cameraPos);
                }

                if (!occluded) {
                    result.visibleEntities.push_back(prim.entity);
                }
                result.primitivesVisited++;
            }
        } else {
            // Add children to stack with depth ranges
            if (node.leftChild != 0) {
                float minDepth, maxDepth;
                computeNodeDepthRange(node.leftChild, cameraPos, minDepth, maxDepth, frameIndex);
                nodeStack[frameIndex].push_back({ node.leftChild, minDepth, maxDepth });
            }
            if (node.rightChild != 0) {
                float minDepth, maxDepth;
                computeNodeDepthRange(node.rightChild, cameraPos, minDepth, maxDepth, frameIndex);
                nodeStack[frameIndex].push_back({ node.rightChild, minDepth, maxDepth });
            }
        }
    }

    return; //result;
}

DualBVH::TraversalResult DualBVH::broadPhaseCollision(uint8_t index) const {
    TraversalResult result;
    if (isEmpty(index)) return result;

    // Self-collision detection using recursive traversal
    std::function<void(uint32_t, uint32_t, uint8_t)> traverse =
        [&](uint32_t nodeA, uint32_t nodeB, uint8_t frameIndex)
        {

        const BVHNode& a = nodes[frameIndex][nodeA];
        const BVHNode& b = nodes[frameIndex][nodeB];

        result.nodesVisited++;

        if (!a.bounds.intersects(b.bounds)) return;

        if (a.isLeaf() && b.isLeaf()) {
            // Test all primitive pairs
            for (uint32_t i = 0; i < a.primitiveCount; ++i) {
                for (uint32_t j = (nodeA == nodeB ? i + 1 : 0); j < b.primitiveCount; ++j) {
                    uint32_t primA = -1;// primitiveIndices[frameIndex][a.firstPrimitive + i];
                    uint32_t primB = -1; // primitiveIndices[frameIndex][b.firstPrimitive + j];



                    if (primitives[frameIndex][primA].bounds.getCoarseAABB().intersects(
                            primitives[frameIndex][primB].bounds.getCoarseAABB())) {
                        result.collisionPairs.emplace_back(
                            primitives[frameIndex][primA].entity,
                            primitives[frameIndex][primB].entity
                        );
                        result.primitivesVisited += 2;
                    }
                }
            }
        } else if (a.isLeaf()) {
            if (b.leftChild != 0) traverse(nodeA, b.leftChild, frameIndex);
            if (b.rightChild != 0) traverse(nodeA, b.rightChild, frameIndex);
        } else if (b.isLeaf()) {
            if (a.leftChild != 0) traverse(a.leftChild, nodeB, frameIndex);
            if (a.rightChild != 0) traverse(a.rightChild, nodeB, frameIndex);
        } else {
            // Both are internal nodes - test all combinations
            if (a.leftChild != 0 && b.leftChild != 0)
                traverse(a.leftChild, b.leftChild, frameIndex);
            if (a.leftChild != 0 && b.rightChild != 0)
                traverse(a.leftChild, b.rightChild, frameIndex);
            if (a.rightChild != 0 && b.leftChild != 0)
                traverse(a.rightChild, b.leftChild, frameIndex);
            if (a.rightChild != 0 && b.rightChild != 0)
                traverse(a.rightChild, b.rightChild, frameIndex);
        }
        };

    traverse(rootIndex[index], rootIndex[index], index);
    return result;
}
