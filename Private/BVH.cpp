#include "BVH.hpp"
#include "BoundingVolume.hpp"
#include "Transform.hpp"
#include "Frustum.hpp"

uint32_t DualBVH::buildRecursive(std::vector<uint32_t>& primitiveIds, uint32_t depth, uint32_t parent) {
     if (primitiveIds.empty()) return 0;

     uint32_t nodeIndex = ++nodeCount;
     if (nodeIndex >= nodes.size()) {
         nodes.resize(nodeIndex + 1);
     }

     BVHNode& node = nodes[nodeIndex];
     node.parentIndex = parent;
     node.bounds = computeBounds(primitiveIds);
     node.setDirty(false);

     // Leaf node condition
     if (primitiveIds.size() <= settings.maxLeafPrimitives || depth > 32) {
         node.setLeaf(true);
         node.firstPrimitive = primitiveIndices.size();
         node.primitiveCount = primitiveIds.size();

         // Copy primitive indices to the end of the array
         for (uint32_t primId : primitiveIds) {
             primitiveIndices.push_back(primId);
         }

         return nodeIndex;
     }
     // Internal node - find best split
     int bestAxis;
     float bestPos;
     uint32_t bestCost = findBestSplit(primitiveIds, bestAxis, bestPos);

     // Partition primitives
     auto partition = std::partition(primitiveIds.begin(), primitiveIds.end(),
                                     [&](uint32_t primId) {
                                         return primitives[primId].worldBounds.center()[bestAxis] < bestPos;
                                     });

     std::vector<uint32_t> leftPrims(primitiveIds.begin(), partition);
     std::vector<uint32_t> rightPrims(partition, primitiveIds.end());

     // Ensure both sides have primitives
     if (leftPrims.empty() || rightPrims.empty()) {
         // Fallback to median split
         std::sort(primitiveIds.begin(), primitiveIds.end(),
                   [&](uint32_t a, uint32_t b) {
                       return primitives[a].worldBounds.center()[bestAxis] <
                           primitives[b].worldBounds.center()[bestAxis];
                   });

         size_t mid = primitiveIds.size() / 2;
         leftPrims.assign(primitiveIds.begin(), primitiveIds.begin() + mid);
         rightPrims.assign(primitiveIds.begin() + mid, primitiveIds.end());
     }

     node.setLeaf(false);
     node.splitAxis = bestAxis;
     node.leftChild = buildRecursive(leftPrims, depth + 1, nodeIndex);
     node.rightChild = buildRecursive(rightPrims, depth + 1, nodeIndex);

     return nodeIndex;
    }

uint32_t DualBVH::findBestSplit(const std::vector<uint32_t>& primitiveIds, int& bestAxis, float& bestPos) {

    if (settings.useMedianSplit) {
        // Simple median split
        AABB bounds = computeBounds(primitiveIds);
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

    AABB totalBounds = computeBounds(primitiveIds);
    float totalSA = totalBounds.surfaceArea();

    for (int axis = 0; axis < 3; ++axis) {
        // Sort primitives along this axis
        std::vector<uint32_t> sorted = primitiveIds;
        std::sort(sorted.begin(), sorted.end(),
                  [&](uint32_t a, uint32_t b) {
                      return primitives[a].worldBounds.center()[axis] <
                          primitives[b].worldBounds.center()[axis];
                  });

        // Try different split positions
        for (size_t i = 1; i < sorted.size(); ++i) {
            std::vector<uint32_t> leftPrims(sorted.begin(), sorted.begin() + i);
            std::vector<uint32_t> rightPrims(sorted.begin() + i, sorted.end());

            AABB leftBounds = computeBounds(leftPrims);
            AABB rightBounds = computeBounds(rightPrims);

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
                bestPos = (primitives[sorted[i - 1]].worldBounds.center()[axis] +
                           primitives[sorted[i]].worldBounds.center()[axis]) * 0.5f;
            }
        }
    }

    return bestCost;
}

void DualBVH::collectOccluders(const glm::vec3& cameraPos, const Frustum& frustum, std::vector<OccluderData>& occluders) const {

    std::vector<uint32_t> nodeStack;
    nodeStack.reserve(64);
    nodeStack.push_back(rootIndex);

    while (!nodeStack.empty()) {
        uint32_t nodeIndex = nodeStack.back();
        nodeStack.pop_back();

        const BVHNode& node = nodes[nodeIndex];

        if (!MMath::aabbVisible(node.bounds.min, node.bounds.max, frustum)) {
            continue;
        }

        if (node.isLeaf()) {
            for (uint32_t i = 0; i < node.primitiveCount; ++i) {

                // ignore if cam is inside
                if (node.bounds.contains(cameraPos))
                    continue;

                uint32_t primIndex = primitiveIndices[node.firstPrimitive + i];
                const BVHPrimitive& prim = primitives[primIndex];

                auto verts = prim.worldBounds.getVertices();
                auto closest = FLT_MAX;
                for (auto& vert : verts) {
                    auto sqrLen = glm::length2(vert - cameraPos);
                    if (sqrLen < closest)
                        closest = sqrLen;
                }

                auto center = prim.worldBounds.center();
                float depth = closest;//glm::length(center - cameraPos);
                glm::vec3 size = prim.worldBounds.size();

                // Heuristic: large objects close to camera are potential occluders
                float volume = size.x * size.y * size.z;
                bool isOccluder = (volume > 1.0f && depth < 50.0f) || volume > 10.0f;

                occluders.emplace_back(prim.entity, prim.worldBounds, depth, isOccluder);
            }
        } else {
            if (node.leftChild != 0) nodeStack.push_back(node.leftChild);
            if (node.rightChild != 0) nodeStack.push_back(node.rightChild);
        }
    }
}

void DualBVH::build(ArenaRegistry& registry) {
    // Clear existing data
    nodes.clear();
    primitives.clear();
    primitiveIndices.clear();
    entityToPrimitive.clear();
    occluderIndices.clear();

    // Collect all entities with Transform and AABB components
    auto view = registry.view<BoundingVolumeComponent>();
    //primitives.reserve(view.size_hint());
    //primitiveIndices.reserve(view.size_hint());

    uint32_t primIndex = 0;
    for (auto [entity, boundComp] : view.each()) {
        //const auto& transformComp = view.get<TransformComponent>(entity);
        //const auto& boundComp = view.get<BoundingVolumeComponent>(entity);


        BoundingVolume bounds = BoundingVolume{ &registry, entity };
        AABB box = bounds.getCoarseAABB();
        primitives.emplace_back(entity, box);
        primitives.back().worldBounds = box;
        primitives.back().frameUpdated = currentFrame;

        if (boundComp.flags & static_cast<uint32_t>(BoundingVolumeFlags::Occluder))
            occluderIndices.push_back(primIndex);

        entityToPrimitive[entity] = primIndex++;
    }

    if (primitives.empty()) {
        nodeCount = 0;
        rootIndex = 0;
        return;
    }

    // Build primitive indices array
    primitiveIndices.resize(primitives.size());
    std::iota(primitiveIndices.begin(), primitiveIndices.end(), 0);

    // Build tree
    std::vector<uint32_t> allPrimitives(primitives.size());
    std::iota(allPrimitives.begin(), allPrimitives.end(), 0);

    nodes.reserve(primitives.size() * 2);  // Worst case
    nodeCount = 0;
    rootIndex = buildRecursive(allPrimitives, 0, 0);

    dirtyCount = 0;
}

void DualBVH::incrementalUpdate(ArenaRegistry& registry) {
    if (primitives.empty()) {
        build(registry);
        return;
    }

    // Update all dirty primitives
    auto view = registry.view<BoundingVolumeComponent>();
    uint32_t updatedCount = 0;

    for (auto entity : view) {
        auto it = entityToPrimitive.find(entity);
        if (it == entityToPrimitive.end()) {
            // New entity - trigger rebuild
            build(registry);
            return;
        }

        uint32_t primIndex = it->second;
        if (primitives[primIndex].frameUpdated < currentFrame) {
            TransformComponent& transComp = registry.get<TransformComponent>(entity);
            if (!transComp.state.hasFlag(ObjectState::MovedThisFrame))
                continue;
            Transform transform = Transform{ &registry, entity };
            updatePrimitive(primIndex, transform.localToWorld());
            updatedCount++;
        }
    }

    // Check if rebuild is needed
    if (needsRebuild()) {
        rebuild(registry);
    }
}




void DualBVH::frustumCull(DualBVH::TraversalResult& result, const Frustum& f) const {
    //TraversalResult result;
        if (isEmpty()) return; //result;

    std::vector<uint32_t> nodeStack;
    nodeStack.reserve(64);
    nodeStack.push_back(rootIndex);

    while (!nodeStack.empty()) {
        uint32_t nodeIndex = nodeStack.back();
        nodeStack.pop_back();

        const BVHNode& node = nodes[nodeIndex];
        result.nodesVisited++;

        // Test AABB against frustum
        if (!MMath::aabbVisible(node.bounds.min, node.bounds.max, f)) {
            continue;
        }

        if (node.isLeaf()) {
            // Add all primitives in this leaf
            for (uint32_t i = 0; i < node.primitiveCount; ++i) {
                uint32_t primIndex = primitiveIndices[node.firstPrimitive + i];
                result.visibleEntities.push_back(primitives[primIndex].entity);
                result.primitivesVisited++;
            }
        } else {
            // Add children to stack (back-to-front for better culling)
            if (node.rightChild != 0) nodeStack.push_back(node.rightChild);
            if (node.leftChild != 0) nodeStack.push_back(node.leftChild);
        }
    }

    return; //result;
}

void DualBVH::frustumCullWithOcclusion(
    DualBVH::TraversalResult& result,
    const Frustum& frustum,
    const glm::vec3& cameraPos,
    OcclusionMethod method) const {

    //TraversalResult result;
    if (isEmpty()) return; //result;

    // Collect potential occluders first (front-to-back)
    std::vector<OccluderData> potentialOccluders;
    for (auto& index : occluderIndices) {

        auto& prim = primitives[index];

        if (!MMath::aabbVisible(prim.worldBounds.min, prim.worldBounds.max, frustum)) {
            continue;
        }

        auto verts = prim.worldBounds.getVertices();
        auto closest = FLT_MAX;
        for (auto& vert : verts) {
            auto sqrLen = glm::length2(vert - cameraPos);
            if (sqrLen < closest)
                closest = sqrLen;
        }

        potentialOccluders.push_back({prim.entity, prim.worldBounds, closest, true});
    }
    //collectOccluders(cameraPos, frustum, potentialOccluders);
    //for (const auto& occluder : potentialOccluders) {
    //    result.activeOccluders.push_back(occluder.entity);
    //}

    // Sort occluders front-to-back
    //std::sort(potentialOccluders.begin(), potentialOccluders.end(),
    //          [](const OccluderData& a, const OccluderData& b) {
    //              return a.depth < b.depth;
    //              //return a.volume > b.volume;
    //          });

    // Build active occluder set (simplified - you'd want more sophisticated occlusion volumes)
    std::vector<AABB> activeOccluders;
    for (const auto& occluder : potentialOccluders) {
        if (occluder.isOccluder) {                              //////////////////// TODO TODO TODO TODO!!!
            activeOccluders.push_back(occluder.bounds);
            result.activeOccluders.push_back(occluder.entity);
            // Limit number of active occluders for performance
            if (activeOccluders.size() >= 8) break;
        }
    }

    // Traverse with occlusion testing
    struct TraversalNode
    {
        uint32_t nodeIndex;
        float minDepth;
        float maxDepth;
    };

    std::vector<TraversalNode> nodeStack;
    nodeStack.reserve(64);

    float rootMinDepth, rootMaxDepth;
    computeNodeDepthRange(rootIndex, cameraPos, rootMinDepth, rootMaxDepth);
    nodeStack.push_back({ rootIndex, rootMinDepth, rootMaxDepth });

    while (!nodeStack.empty()) {
        TraversalNode current = nodeStack.back();
        nodeStack.pop_back();

        const BVHNode& node = nodes[current.nodeIndex];
        result.nodesVisited++;

        // Frustum test first (cheapest)
        if (!MMath::aabbVisible(node.bounds.min, node.bounds.max, frustum)) {
            result.nodesCulledByFrustum++;
            continue;
        }

        // Occlusion test
        if (method != OcclusionMethod::NONE && !activeOccluders.empty()) {
            if (!node.bounds.contains(cameraPos)) {

                //if (isOccludedRaycast(node.bounds, activeOccluders, cameraPos, current.minDepth)) {
                //    result.nodesCulledByOcclusion++;
                //    continue;
                //}

                for (const auto& occluder : activeOccluders) {
                    if (isOccludedRaycast(node.bounds, occluder, cameraPos) ) {
                        result.nodesCulledByOcclusion++;
                        continue;
                    }
                }
            }
        }

        if (node.isLeaf()) {
            // Process leaf primitives
            for (uint32_t i = 0; i < node.primitiveCount; ++i) {
                uint32_t primIndex = primitiveIndices[node.firstPrimitive + i];
                const BVHPrimitive& prim = primitives[primIndex];

                // Fine-grained occlusion test for individual primitives
                bool occluded = false;
                if (method != OcclusionMethod::NONE && !activeOccluders.empty()) {
                    float primDepth = glm::length(prim.worldBounds.center() - cameraPos);
                    occluded = isOccluded(prim.worldBounds, activeOccluders, cameraPos, primDepth) &&
                        !prim.worldBounds.contains(cameraPos);
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
                computeNodeDepthRange(node.leftChild, cameraPos, minDepth, maxDepth);
                nodeStack.push_back({ node.leftChild, minDepth, maxDepth });
            }
            if (node.rightChild != 0) {
                float minDepth, maxDepth;
                computeNodeDepthRange(node.rightChild, cameraPos, minDepth, maxDepth);
                nodeStack.push_back({ node.rightChild, minDepth, maxDepth });
            }
        }
    }

    return; //result;
}

DualBVH::TraversalResult DualBVH::broadPhaseCollision() const {
    TraversalResult result;
    if (isEmpty()) return result;

    // Self-collision detection using recursive traversal
    std::function<void(uint32_t, uint32_t)> traverse =
        [&](uint32_t nodeA, uint32_t nodeB) {
        const BVHNode& a = nodes[nodeA];
        const BVHNode& b = nodes[nodeB];

        result.nodesVisited++;

        if (!a.bounds.intersects(b.bounds)) return;

        if (a.isLeaf() && b.isLeaf()) {
            // Test all primitive pairs
            for (uint32_t i = 0; i < a.primitiveCount; ++i) {
                for (uint32_t j = (nodeA == nodeB ? i + 1 : 0); j < b.primitiveCount; ++j) {
                    uint32_t primA = primitiveIndices[a.firstPrimitive + i];
                    uint32_t primB = primitiveIndices[b.firstPrimitive + j];

                    if (primitives[primA].worldBounds.intersects(primitives[primB].worldBounds)) {
                        result.collisionPairs.emplace_back(
                            primitives[primA].entity,
                            primitives[primB].entity
                        );
                        result.primitivesVisited += 2;
                    }
                }
            }
        } else if (a.isLeaf()) {
            if (b.leftChild != 0) traverse(nodeA, b.leftChild);
            if (b.rightChild != 0) traverse(nodeA, b.rightChild);
        } else if (b.isLeaf()) {
            if (a.leftChild != 0) traverse(a.leftChild, nodeB);
            if (a.rightChild != 0) traverse(a.rightChild, nodeB);
        } else {
            // Both are internal nodes - test all combinations
            if (a.leftChild != 0 && b.leftChild != 0)
                traverse(a.leftChild, b.leftChild);
            if (a.leftChild != 0 && b.rightChild != 0)
                traverse(a.leftChild, b.rightChild);
            if (a.rightChild != 0 && b.leftChild != 0)
                traverse(a.rightChild, b.leftChild);
            if (a.rightChild != 0 && b.rightChild != 0)
                traverse(a.rightChild, b.rightChild);
        }
        };

    traverse(rootIndex, rootIndex);
    return result;
}
