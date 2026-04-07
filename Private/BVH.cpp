#include "BVH.hpp"
#include "BoundingVolume.hpp"
#include "Transform.hpp"
#include "Frustum.hpp"
#include "MJob.hpp"
#include "Profiler.hpp"
#include "Engine.h"
#include "Scene.h"
#include "BoundingSystem.h"



void DualBVH::_tSafe_insertionSort(uint32_t* primitiveIds, const uint32_t primCount, const int bestAxis, const uint8_t index) {
    uint32_t* first = primitiveIds;
    const uint32_t* last = primitiveIds + primCount;
    if (last - first <= 1) {
        // nothing to do
    } else {
        const auto axis = static_cast<int>(bestAxis);
        auto& prims = primitives[index]; // local alias to avoid capturing outer state repeatedly

        for (uint32_t* it = first + 1; it < last; ++it) {
            const uint32_t id = *it;

            AABB& bounds = prims[id].bounds;//GET_AABB(prims[id]);

                // key to insert: center value along axis
                const float keyVal = bounds.center()[axis];

            uint32_t* jt = it;
            // shift larger elements one step to the right
            while (jt > first) {
                const uint32_t prevId = *(jt - 1);
                AABB& prevBounds = prims[prevId].bounds;//GET_AABB(prims[prevId]);

                const float    prevVal = prevBounds.center()[axis];

                // Stable insertion: stop when !(key < prev)
                if (!(keyVal < prevVal)) break;

                *jt = prevId;   // move prev one step to the right
                --jt;
            }
            *jt = id;           // place key
        }
    }
}

void DualBVH::_updatePrimitives(DualBVH* _this) {

    while (true) {

        _this->primitivesStartSema.acquire();
        if (!_this->workersRunning.load(std::memory_order_acquire))
            return;
        {
            PROFILE_SCOPE("BVH_RebuildPrimitiveAABBs");
            const auto idx = _this->bufferIdx.load(std::memory_order_acquire);
            if (idx == UINT8_INVALID)
                continue;

            auto group = _this->m_reg.group<TransformComponent, BoundingVolumeComponent, EnabledTag>();
            auto& prims = _this->primitives[idx];
            MJob::for_loop(0, prims.size(), 6,
                            [&](size_t i) {
                                auto& prim = prims[i];
                                const auto& trans = group.get<TransformComponent>(prim.entity);

                                if (trans.state.hasFlag(ObjectState::ParentMovedThisFrame) ||
                                    trans.state.hasFlag(ObjectState::MovedThisFrame)) {

                                    const auto& bounds = group.get<BoundingVolumeComponent>(prim.entity);
                                    prim.bounds = Engine::getInstance()->getScene(trans.sceneIndex)->
                                        boundingSystem().aabbs()[bounds.coarseIndexWorld];
                                    prim.center = prim.bounds.center();
                                }

                            });
            //_this->primitivesLock[0].release();







            // if (_this->primitivesLock[0].try_acquire()) {
            //     auto group = _this->m_reg.group<TransformComponent, BoundingVolumeComponent, EnabledTag>();
            //     auto& prims0 = _this->primitives[0];
            //     MJob::for_loop(0, prims0.size(), 6,
            //                     [&](size_t i) {
            //                         auto& prim = prims0[i];
            //                         const auto& trans = group.get<TransformComponent>(prim.entity);
            //
            //                         if (trans.state.hasFlag(ObjectState::ParentMovedThisFrame) ||
            //                             trans.state.hasFlag(ObjectState::MovedThisFrame)) {
            //
            //                             const auto& bounds = group.get<BoundingVolumeComponent>(prim.entity);
            //                             prim.bounds = Engine::getInstance()->getScene(trans.sceneIndex)->
            //                                 boundingSystem().aabbs()[bounds.coarseIndexWorld];
            //                             prim.center = prim.bounds.center();
            //                         }
            //
            //                     });
            //     _this->primitivesLock[0].release();
            //     continue;
            // }
            //
            // if (_this->primitivesLock[1].try_acquire()) {
            //     auto group = _this->m_reg.group<TransformComponent, BoundingVolumeComponent, EnabledTag>();
            //     auto& prims1 = _this->primitives[1];
            //     MJob::for_loop(0, prims1.size(), 6,
            //                     [&](size_t i) {
            //                         auto& prim = prims1[i];
            //                         const auto& trans = group.get<TransformComponent>(prim.entity);
            //
            //                         if (trans.state.hasFlag(ObjectState::ParentMovedThisFrame) ||
            //                             trans.state.hasFlag(ObjectState::MovedThisFrame)) {
            //
            //                             const auto& bounds = group.get<BoundingVolumeComponent>(prim.entity);
            //                             prim.bounds = Engine::getInstance()->getScene(trans.sceneIndex)->
            //                                 boundingSystem().aabbs()[bounds.coarseIndexWorld];
            //                             prim.center = prim.bounds.center();
            //                         }
            //
            //                     });
            //     _this->primitivesLock[1].release();
            //     continue;
            // }

        }
    }
}

void DualBVH::_recursiveWorker(DualBVH* _this, uint8_t threadId) {
    const auto id = static_cast<size_t>(threadId);
    while (true) {
        
        _this->startSemas[id]->acquire();

        {
            PROFILE_SCOPE("BVH_BuildWorker");
            if (!_this->workersRunning.load(std::memory_order_acquire))
                return;

            while (_this->workPkgCondition[id] == false) {}

            //_this->workPkgSemas[id]->acquire();
            WorkerPkg& pkg = *_this->workerPkgs[id];
            const auto result = _this->buildRecursive(pkg.primitiveIds.data(), pkg.primCount, pkg.depth, pkg.parent, pkg.index);
            _this->workerResults[id].store(result, std::memory_order_release);
            _this->workPkgCondition[id].store(false);
        }
        _this->doneSemas[id]->release();
    }
}

void DualBVH::_buildThreadMethod(DualBVH* _this, ArenaRegistry& registry) {


    while (true) {


        _this->_threadStart.acquire();

        {
            PROFILE_SCOPE("BVH_BuildSink");
            if (!_this->workersRunning.load(std::memory_order_acquire)) {
                return;
            }

            uint8_t idx;
            const uint8_t readBufIdx = _this->bufferIdx.load(std::memory_order_acquire);
            if (readBufIdx == UINT8_INVALID)
                idx = 0;
            else {
                idx = (readBufIdx + 1) % BVH_BUFFERS;
            }


            auto group = registry.group<TransformComponent, BoundingVolumeComponent, EnabledTag>();
            _this->m_groupPtr.store(&group, std::memory_order_release);
            for (auto entity : group) {

                auto it = _this->entityToPrimitive[idx].find(entity);
                if (it == _this->entityToPrimitive[idx].end()) {
                    //_this->primitivesLock[index].acquire();
                    _this->rebuildPrimitives(registry, idx);
                    //this->primitivesLock[index].release();
                    break;
                }
            }
            
            _this->buildNodes(registry, idx);


            const auto newIdx = (idx) % BVH_BUFFERS;
            _this->bufferIdx.store(newIdx, std::memory_order_release);
        }
        _this->_threadDone.release();
    }
}

void DualBVH::rebuildPrimitives(ArenaRegistry&, const uint8_t index) {
    // Clear existing data
    primitives[index].clear();
    //primitiveIndices[index].clear();
    entityToPrimitive[index].clear();
    alwaysVisible[index].clear();

    // Collect all entities with Transform and AABB components
    //auto view = registry.view<BoundingVolumeComponent>();
    uint32_t primIndex = 0;
    const auto group = m_groupPtr.load(std::memory_order_acquire);
    for (auto [entity, trans, boundComp, tag] : group->each()) {

        if (boundComp.flags & static_cast<uint32_t>(BoundingVolumeFlags::Occluder) || 
            boundComp.flags & static_cast<uint32_t>(BoundingVolumeFlags::Occludee)) {

            const AABB& bounds = Engine::getInstance()->getScene(trans.sceneIndex)->boundingSystem().aabbs()[boundComp.coarseIndexWorld];

            //BoundingVolume bounds = BoundingVolume{ &registry, entity };
            //primitives[index].emplace_back(entity, boundComp.coarseIndexWorld, trans.sceneIndex);
            primitives[index].emplace_back(entity, bounds);

            if (boundComp.flags & static_cast<uint32_t>(BoundingVolumeFlags::Occluder)) {
                const auto cornerIndex = occluderCorners.size();
                auto verts = bounds.getVertices();
                occluderCorners[index].insert(occluderCorners[index].end(), verts.begin(), verts.end());
                occluderIndices[index].emplace_back(primIndex, static_cast<uint32_t>(cornerIndex), 0.0f);
            }

            entityToPrimitive[index][entity] = primIndex++;
        } else {
            alwaysVisible[index].push_back(entity);
        }


    }

    // ReSharper disable once CppIfCanBeReplacedByConstexprIf
    if (primitives.empty()) {
        nodeCount[index].store(0);
        rootIndex[index].store(0);
        return;
    }

    // Build primitive indices array
    //primitiveIndices[index].resize(primitives[index].size());
    //std::iota(primitiveIndices[index].begin(), primitiveIndices[index].end(), 0);
}

uint32_t DualBVH::buildRecursive(
    uint32_t* primitiveIds,
    const uint32_t primCount, const uint8_t depth, const uint32_t parent, const uint8_t index) {
     if (primCount == 0) return 0;

     const uint32_t nodeIndex = nodeCount[index].fetch_add(1, std::memory_order_acquire);

     BVHNode& node = nodes[index][nodeIndex];


     node.parentIndex = parent;
     const auto bounds = computeBounds(primitiveIds, primCount, index);
     //node.center = bounds.center();
     //node.extent = bounds.extent();
     node.bounds = bounds;
     node.depth = depth;
     node.setDirty(false);

     // Leaf node condition
     if (primCount <= settings.maxLeafPrimitives) { //|| depth > settings.maxDepth) {
         node.setLeaf(true);

         node.primCount = static_cast<uint8_t>(primCount);

         for (size_t i = 0; i < primCount; ++i) {
             node.primIndices[i] = primitiveIds[i];
         }

         return nodeIndex;
     }
     // Internal node - find best split
     int bestAxis;
     float bestPos;
     [[maybe_unused]] uint32_t bestCost = findBestSplit(primitiveIds, primCount, bestAxis, bestPos, index);


     




     //auto& prims = primitives[index];
     //const uint32_t axis = static_cast<uint32_t>(bestAxis);

     //auto key = [&](uint32_t id) -> float {
     //    return prims[id].bounds.center()[axis];
     //    };

     //uint32_t* begin = primitiveIds;
     //uint32_t* end = primitiveIds + primCount;

     //// Measure centroid extent along axis
     //float cmin = std::numeric_limits<float>::infinity();
     //float cmax = -cmin;
     //for (uint32_t* p = begin; p != end; ++p) {
     //    float c = key(*p);
     //    cmin = std::min(cmin, c);
     //    cmax = std::max(cmax, c);
     //}

     //ArenaVector<uint32_t> leftPrims{ ArenaAllocator<uint32_t>(_frameArena) };
     //ArenaVector<uint32_t> rightPrims{ ArenaAllocator<uint32_t>(_frameArena) };
     //
     //// If too little extent, don’t waste time sorting—just split evenly.
     //constexpr float kEps = 1e-9f;
     //if (cmax - cmin <= kEps) {
     //    uint32_t* mid = begin + primCount / 2;   // balanced by count
     //    leftPrims.assign(begin, mid);
     //    rightPrims.assign(mid, end);
     //} else {
     //    // 1) Try SAH pivot (bestPos). Clamp it a bit inside the range to avoid empties.
     //    float pivot = std::clamp(bestPos, std::nextafter(cmin, cmax), std::nextafter(cmax, cmin));

     //    uint32_t* cut = std::partition(begin, end, [&](uint32_t id) {
     //        return key(id) < pivot;              // strict; equal-to-pivot go to right
     //                                   });

     //    size_t leftCount = static_cast<size_t>(cut - begin);
     //    size_t rightCount = static_cast<size_t>(end - cut);

     //    // 2) If split is empty or too unbalanced, median split with nth_element (O(n))
     //    //    This guarantees both sides non-empty and keeps the tree healthy.
     //    if (leftCount == 0 || rightCount == 0) {
     //        uint32_t* mid = begin + primCount / 2;
     //        std::nth_element(begin, mid, end, [&](uint32_t a, uint32_t b) {
     //            float ka = key(a), kb = key(b);
     //            if (ka == kb) return a < b;      // tie-breaker for strict weak ordering
     //            return ka < kb;
     //                         });
     //        leftPrims.assign(begin, mid);
     //        rightPrims.assign(mid, end);
     //    } else {
     //        leftPrims.assign(begin, cut);
     //        rightPrims.assign(cut, end);
     //    }
     //}



//     ArenaVector<uint32_t> leftPrims{ ArenaAllocator<uint32_t>(_frameArena) };
//     ArenaVector<uint32_t> rightPrims{ ArenaAllocator<uint32_t>(_frameArena) };
//     constexpr float kEps = 1e-9f;
//     if (cmax - cmin <= kEps) {
//         uint32_t* mid = begin + primCount / 2;   // balanced by count
//         leftPrims.assign(begin, mid);
//         rightPrims.assign(mid, end);
//     } else {
//
//         float pivot = bestPos;
//
//         // If the SAH pivot is unusable or outside the open interval, skip straight to median.
//         if (!std::isfinite(pivot) || !(pivot > cmin && pivot < cmax)) {
//             goto MEDIAN_SPLIT;
//         }
//
//         uint32_t* cut = std::partition(begin, end, [&](uint32_t id) {
//             return key(id) < pivot;  // strict: equals go right
//                                        });
//
//         if (cut == begin || cut == end) {
//             // empty side → use median
//             goto MEDIAN_SPLIT;
//         }
//
//         // good split
//         leftPrims.assign(begin, cut);
//         rightPrims.assign(cut, end);
//         goto DONE;
//     }
//MEDIAN_SPLIT:
//     {
//         uint32_t* mid = begin + primCount / 2;
//         std::nth_element(begin, mid, end, [&](uint32_t a, uint32_t b) {
//             float ka = key(a), kb = key(b);
//             if (ka == kb) return a < b;
//             return ka < kb;
//                          });
//         leftPrims.assign(begin, mid);
//         rightPrims.assign(mid, end);
//     }
//DONE:;






    //ArenaVector<uint32_t> leftPrims{ ArenaAllocator<uint32_t>(_frameArena) };
    ////leftPrims.resize(primCount);
    //ArenaVector<uint32_t> rightPrims{ ArenaAllocator<uint32_t>(_frameArena) };
    ////rightPrims.resize(primCount);
    //uint32_t leftCount = 0;
    //uint32_t rightCount = 0;
    //
    //// First pass: count
    //for (uint32_t i = 0; i < primCount; ++i) {
    //    if (primitives[index][primitiveIds[i]].center[bestAxis] < bestPos) {
    //        leftCount++;
    //    } else {
    //        rightCount++;
    //    }
    //}
    //
    ////// Second pass: separate (or use pre-allocated thread-local buffers)
    ////uint32_t* leftPrims = threadLocalLeft[threadId];   // Pre-allocated
    ////uint32_t* rightPrims = threadLocalRight[threadId]; // Pre-allocated
    //
    ////uint32_t leftIdx = 0, rightIdx = 0;
    //for (uint32_t i = 0; i < primCount; ++i) {
    //    if (primitives[index][primitiveIds[i]].center[bestAxis] < bestPos) {
    //        leftPrims.push_back(primitiveIds[i]);
    //    } else {
    //        rightPrims.push_back(primitiveIds[i]);
    //    }
    //}




     // Partition primitives
     const auto partition = std::partition(primitiveIds, primitiveIds + primCount,
                                     [&](uint32_t primId) {
                                         //AABB& bounds = primitives[index][primId].bounds;//GET_AABB(primitives[index][primId]);                                      
                                         //return bounds.center()[bestAxis] < bestPos;

                                         return primitives[index][primId].center[bestAxis] < bestPos;
                                     });

    

    thread_local std::vector<uint32_t>leftPrims;
    leftPrims.reserve(primCount);
    leftPrims.clear();
    leftPrims.assign(primitiveIds, partition);

    thread_local std::vector<uint32_t>rightPrims;
    rightPrims.reserve(primCount);
    rightPrims.clear();
    rightPrims.assign(partition, partition + primCount);




    //ArenaVector<uint32_t> leftPrims(primitiveIds, partition, ArenaAllocator<uint32_t>(_frameArena));
    //ArenaVector<uint32_t> rightPrims(partition, primitiveIds + primCount, ArenaAllocator<uint32_t>(_frameArena));


     // Ensure both sides have primitives
     if (leftPrims.empty() || rightPrims.empty()) {

         //_tSafe_insertionSort(primitiveIds, primCount, bestAxis, index);

         // Fallback to median split
         //std::sort(primitiveIds, primitiveIds + primCount,
         //          [&](uint32_t a, uint32_t b) {

         //              auto A = primitives[index][a].bounds.getCoarseAABB();
         //              auto B = primitives[index][b].bounds.getCoarseAABB();

         //              return A.center()[bestAxis] <
         //                  B.center()[bestAxis];
         //          });

         const size_t mid = primCount / 2;
         leftPrims.assign(primitiveIds, primitiveIds + mid);
         rightPrims.assign(primitiveIds + mid, primitiveIds + primCount);
     }

     node.setLeaf(false);
     node.splitAxis = static_cast<uint8_t>(bestAxis);
     const auto maxThreadDepthLevel = static_cast<uint32_t>(std::floor(std::log2(NUM_BUILD_THREADS)));
     if (depth < maxThreadDepthLevel && nodes[index].size() / NUM_BUILD_THREADS >= NUM_BUILD_THREADS) {

         const auto idL = _getNextThreadId();
         //workerPrimsTemp[idL].resize(leftPrims.size());
         //std::memcpy(workerPrimsTemp[idL].data(), leftPrims.data(), leftPrims.size() * sizeof(uint32_t));
         WorkerPkg l_pkg{ _frameArena };
         //l_pkg.primitiveIds = &workerPrimsTemp[idL];
         l_pkg.primitiveIds = leftPrims;
         l_pkg.primCount = static_cast<uint32_t>(leftPrims.size());
         l_pkg.depth = depth + 1;
         l_pkg.index = index;
         l_pkg.parent = nodeIndex;
         workerPkgs[idL] = &l_pkg;
         
         //workPkgSemas[idL]->release();
         startSemas[idL]->release();
         workPkgCondition[idL].store(true, std::memory_order_release);

         const auto idR = _getNextThreadId();
         //workerPrimsTemp[idR].resize(rightPrims.size());
         //std::memcpy(workerPrimsTemp[idR].data(), rightPrims.data(), rightPrims.size() * sizeof(uint32_t));
         WorkerPkg r_pkg{ _frameArena };
         //r_pkg.primitiveIds = &workerPrimsTemp[idR];
         r_pkg.primitiveIds = rightPrims;
         r_pkg.primCount = static_cast<uint32_t>(rightPrims.size());
         r_pkg.depth = depth + 1;
         r_pkg.index = index;
         r_pkg.parent = nodeIndex;
         workerPkgs[idR] = &r_pkg;
         
         //workPkgSemas[idR]->release();
         startSemas[idR]->release();
         workPkgCondition[idR].store(true, std::memory_order_release);

         doneSemas[idL]->acquire();
         doneSemas[idR]->acquire();
         node.leftChild = workerResults[idL].load();
         node.rightChild = workerResults[idR].load();


     } else {
         node.leftChild = buildRecursive(leftPrims.data(), static_cast<uint32_t>(leftPrims.size()), depth + 1, nodeIndex, index);
         node.rightChild = buildRecursive(rightPrims.data(), static_cast<uint32_t>(rightPrims.size()), depth + 1, nodeIndex, index);
     }

     return nodeIndex;
    }

uint32_t DualBVH::findBestSplit(const uint32_t* primitiveIds, uint32_t primCount, int& bestAxis, float& bestPos, uint8_t index) {

    if (settings.useMedianSplit) {
        // Simple median split
        AABB bounds = computeBounds(primitiveIds, primCount, index);
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

    AABB totalBounds = computeBounds(primitiveIds, primCount, index);
    float totalSA = totalBounds.surfaceArea();

    for (int axis = 0; axis < 3; ++axis) {
        // Sort primitives along this axis
        ArenaVector<uint32_t> sorted{ ArenaAllocator<uint32_t>{_frameArena} };
        sorted.resize(primCount);
        std::memcpy(sorted.data(), primitiveIds, primCount * sizeof(uint32_t));
        std::ranges::sort(sorted,
                          [&](const uint32_t a, const uint32_t b) {

                              const AABB& A = primitives[index][a].bounds;//GET_AABB(primitives[index][a]);
                              const AABB& B = primitives[index][b].bounds;//GET_AABB(primitives[index][b]);

                              return A.center()[axis] <
                                     B.center()[axis];
                          });

        // Try different split positions
        for (size_t i = 1; i < sorted.size(); ++i) {
            ArenaVector<uint32_t> leftPrims(sorted.begin(),
                sorted.begin() + static_cast<uint32_t>(i), ArenaAllocator<uint32_t>{_frameArena});
            ArenaVector<uint32_t> rightPrims(
                sorted.begin() + static_cast<uint32_t>(i), sorted.end(), ArenaAllocator<uint32_t>{_frameArena});

            AABB leftBounds = computeBounds(leftPrims.data(), static_cast<uint32_t>(leftPrims.size()), index);
            AABB rightBounds = computeBounds(rightPrims.data(), static_cast<uint32_t>(rightPrims.size()), index);

            float leftSA = leftBounds.surfaceArea();
            float rightSA = rightBounds.surfaceArea();

            // SAH cost = traversal_cost + P(left) * cost(left) + P(right) * cost(right)
            const auto cost = static_cast<uint32_t>(
                1.0f + leftSA / totalSA * static_cast<float>(leftPrims.size()) +
                rightSA / totalSA * static_cast<float>(rightPrims.size())
                );

            if (cost < bestCost) {
                bestCost = cost;
                bestAxis = axis;

                AABB& A = primitives[index][sorted[i - 1]].bounds;//GET_AABB(primitives[index][sorted[i - 1]]);
                AABB& B = primitives[index][sorted[i]].bounds;//GET_AABB(primitives[index][sorted[i]]);

                bestPos = (A.center()[axis] +
                           B.center()[axis]) * 0.5f;
            }
        }
    }

    return bestCost;
}

AABB DualBVH::computeBounds(const uint32_t* primitiveIds, const uint32_t primCount, const uint8_t index) const {
    //if (primCount == 0) {
    //    return AABB();
    //}

    //AABB& bounds = primitives[index][primitiveIds[0]].bounds;//GET_AABB(primitives[index][primitiveIds[0]]);//primitives[index][primitiveIds[0]].bounds;
    //for (size_t i = 1; i < primCount; ++i) {
    //    AABB& otherBounds = primitives[index][primitiveIds[i]].bounds;//GET_AABB(primitives[index][primitiveIds[i]]);
    //    bounds = bounds.merge(otherBounds);//     primitives[index][primitiveIds[i]].bounds);
    //}

    //return bounds;

    if (primCount == 0) return AABB{};

    const AABB& firstBounds = primitives[index][primitiveIds[0]].bounds;

    //// Load first bounds into SIMD registers
    //__m128 minVec = _mm_load_ps(&firstBounds.min.x);  // x,y,z,w
    //__m128 maxVec = _mm_load_ps(&firstBounds.max.x);

    //for (uint32_t i = 1; i < primCount; ++i) {
    //    const AABB& bounds = primitives[index][primitiveIds[i]].bounds;

    //    __m128 currentMin = _mm_load_ps(&bounds.min.x);
    //    __m128 currentMax = _mm_load_ps(&bounds.max.x);

    //    minVec = _mm_min_ps(minVec, currentMin);
    //    maxVec = _mm_max_ps(maxVec, currentMax);
    //}

    //glm::vec3 resultMin, resultMax;
    //_mm_store_ps(&resultMin.x, minVec);
    //_mm_store_ps(&resultMax.x, maxVec);

    //return AABB(resultMin, resultMax);

    glm::vec3 minBounds = firstBounds.min;
    glm::vec3 maxBounds = firstBounds.max;

    for (uint32_t i = 1; i < primCount; ++i) {
        const AABB& bounds = primitives[index][primitiveIds[i]].bounds;
        minBounds = glm::min(minBounds, bounds.min);  // GLM vectorized min
        maxBounds = glm::max(maxBounds, bounds.max);  // GLM vectorized max
    }

    return AABB{ minBounds, maxBounds };

}

// void DualBVH::collectOccluders(const glm::vec3& cameraPos, const Frustum& frustum, std::vector<OccluderData>& occluders) const {
//
//     //std::vector<uint32_t> nodeStack;
//     //nodeStack.reserve(64);
//     //nodeStack.push_back(rootIndex);
//
//     //while (!nodeStack.empty()) {
//     //    uint32_t nodeIndex = nodeStack.back();
//     //    nodeStack.pop_back();
//
//     //    const BVHNode& node = nodes[nodeIndex];
//
//     //    if (!MMath::aabbVisible(node.bounds.min, node.bounds.max, frustum)) {
//     //        continue;
//     //    }
//
//     //    if (node.isLeaf()) {
//     //        for (uint32_t i = 0; i < node._pad0; ++i) {
//
//     //            // ignore if cam is inside
//     //            if (node.bounds.contains(cameraPos))
//     //                continue;
//
//     //            uint32_t primIndex = primitiveIndices[node.firstPrimitive + i];
//     //            const BVHPrimitive& prim = primitives[primIndex];
//
//     //            auto verts = prim.worldBounds.getVertices();
//     //            auto closest = FLT_MAX;
//     //            for (auto& vert : verts) {
//     //                auto sqrLen = glm::length2(vert - cameraPos);
//     //                if (sqrLen < closest)
//     //                    closest = sqrLen;
//     //            }
//
//     //            auto center = prim.worldBounds.center();
//     //            float depth = closest;//glm::length(center - cameraPos);
//     //            glm::vec3 size = prim.worldBounds.size();
//
//     //            // Heuristic: large objects close to camera are potential occluders
//     //            float volume = size.x * size.y * size.z;
//     //            bool isOccluder = (volume > 1.0f && depth < 50.0f) || volume > 10.0f;
//
//     //            occluders.emplace_back(prim.entity, prim.worldBounds, depth, isOccluder);
//     //        }
//     //    } else {
//     //        if (node.leftChild != 0) nodeStack.push_back(node.leftChild);
//     //        if (node.rightChild != 0) nodeStack.push_back(node.rightChild);
//     //    }
//     //}
// }

bool DualBVH::isOccludedRaycast(
    const AABB& bounds,
    const std::vector<OccIndexCornerIndexDepthTuple>& occluders,
    const glm::vec3& cameraPos,
    const float objectDepth, const uint8_t index) const {

    // Ray-based occlusion test - much more accurate!
    const auto objectCorners = bounds.getVertices();

    // Test rays from camera to each corner of the object
    int visibleCorners = 0;

    for (int i = 0; i < 8; ++i) {

        glm::vec3 rayDir = glm::normalize(objectCorners[i] - cameraPos);
        const float rayLength = glm::length2(objectCorners[i] - cameraPos);

        bool rayBlocked = false;

        // Test this ray against all occluders
        for (const auto& occluder : occluders) {
            //glm::vec3 occluderCenter = occluder.center();
            //float occluderDepth = glm::length2(occluderCenter - cameraPos);

            // Only test occluders that are closer than the object
            if (std::get<2>(occluder) >= objectDepth - 0.1f) continue;

            // Ray-AABB intersection test
            float tNear, tFar;
            const AABB& box = primitives[index][std::get<0>(occluder)].bounds;//GET_AABB(primitives[index][std::get<0>(occluder)]);//primitives[index][std::get<0>(occluder)].bounds;
            if (rayAABBIntersectWithDistance(cameraPos, rayDir, box, tNear, tFar)) {
                // Check if intersection is between camera and object corner
                if (tNear > 0.01f && tNear < rayLength - 0.01f) {
                    rayBlocked = true;
                    break;
                }
            }
        }

        if (!rayBlocked) {
            visibleCorners++;
            // If any corner is visible, object is not fully occluded
            // You can adjust this threshold - maybe require 2+ visible corners
            if (visibleCorners >= 1) {
                return false;
            }
        }
    }

    // All corners are blocked - object is occluded
    return true;
}



void DualBVH::buildNodes(ArenaRegistry&, const uint8_t index) {
    // Clear existing data
    primitiveIndices[index].clear();

    occluderIndices[index].clear();
    occluderCorners[index].clear();

    // Build tree
    ArenaVector<uint32_t> allPrimitives{ ArenaAllocator<uint32_t>{_frameArena} };
    allPrimitives.resize(primitives[index].size());
    std::iota(allPrimitives.begin(), allPrimitives.end(), 0);

    primitiveIndices[index].resize(primitives[index].size());
    nodes[index].resize(primitives[index].size() * 2);  // Worst case
    nodeCount[index].store(0);
    indicesCount[index].store(0);
    rootIndex[index] = buildRecursive(allPrimitives.data(), static_cast<uint32_t>(allPrimitives.size()), 0, 0, index);

}

void DualBVH::incrementalUpdate(ArenaRegistry& registry, const uint8_t index) {


    if (primitives[index].empty()) {
        rebuildPrimitives(registry, index);
        buildNodes(registry, index);
        return;
    }

    // Update all dirty primitives
    const auto view = registry.view<BoundingVolumeComponent>();
    //uint32_t updatedCount = 0;

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
//            for (uint32_t i = 0; i < node._pad0; ++i) {
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
    TraversalResult& result,
    const Frustum& f,
    const glm::vec3& cameraPos,
    const glm::vec3& camWorldForward,
    const float& camForwardPosDot,
    ArenaRegistry* const registry,
    const OcclusionMethod method) {

    const auto dbuffIdx = bufferIdx.load();

    if (dbuffIdx == UINT8_INVALID)
        return;



    for (auto& entity : alwaysVisible[dbuffIdx])
        result.visibleEntities.push_back(entity);

    if (isEmpty(dbuffIdx))
        return;
    
    cullArenas[dbuffIdx][NUM_RUN_THREADS]->reset();

    // Find occluders
    FrameArenaVector<OccluderData> potentialOccluders{ FrameArenaAllocator<OccluderData>{cullArenas[dbuffIdx][NUM_RUN_THREADS]} };

    for (const auto& [thisIndex, offset, depth] : occluderIndices[dbuffIdx]) {

        auto& prim = primitives[dbuffIdx][thisIndex];
        AABB& worldBounds = prim.bounds;//GET_AABB(prim);
        if (!MMath::aabbVisible(worldBounds.min, worldBounds.max, f)) {
            continue;
        }
        const float d[8] = {
             glm::length2(occluderCorners[dbuffIdx][offset + 0] - cameraPos),
             glm::length2(occluderCorners[dbuffIdx][offset + 1] - cameraPos),
             glm::length2(occluderCorners[dbuffIdx][offset + 2] - cameraPos),
             glm::length2(occluderCorners[dbuffIdx][offset + 3] - cameraPos),
             glm::length2(occluderCorners[dbuffIdx][offset + 4] - cameraPos),
             glm::length2(occluderCorners[dbuffIdx][offset + 5] - cameraPos),
             glm::length2(occluderCorners[dbuffIdx][offset + 6] - cameraPos),
             glm::length2(occluderCorners[dbuffIdx][offset + 7] - cameraPos),
        };
        const __m256 v = _mm256_load_ps(d);
        float mn;
        MMath::hmin8(v, mn);
        float closest = mn;

        potentialOccluders.emplace_back(prim.entity, closest, offset, thisIndex);
    }

    //collectOccluders(cameraPos, frustum, potentialOccluders);
    //for (const auto& occluder : potentialOccluders) {
    //    result.activeOccluders.push_back(occluder.entity);
    //}

    // Sort occluders front-to-back
    std::ranges::sort(potentialOccluders,
                      [](const OccluderData& a, const OccluderData& b) {
                          return a.depth < b.depth;
                          //return a.volume > b.volume;
                      });

    // Build active occluder set (simplified - you'd want more sophisticated occlusion volumes)
    std::vector<OccIndexCornerIndexDepthTuple> activeOccluders;
    for (const auto& occluder : potentialOccluders) {                           //////////////////// TODO TODO TODO TODO!!!
        activeOccluders.emplace_back(occluder.primIndex, occluder.cornersOffset, occluder.depth );
        result.activeOccluders.push_back(occluder.entity);
        // Limit number of active occluders for performance
        if (activeOccluders.size() >= 12) break;

    }

    // Set up MT
    static std::vector<Job> frontier[BVH_BUFFERS];
    frontier[dbuffIdx].reserve(1024);
    frontier[dbuffIdx].clear();

    static constexpr size_t stackSize = 1024;

    static std::array<Job, stackSize> stack[BVH_BUFFERS];
    size_t sp = 0;
    stack[dbuffIdx][sp++] = { rootIndex[dbuffIdx] , 0xf3, 1 };

    const size_t targetDepth = std::min(
        static_cast<unsigned long long>(nodes[dbuffIdx].size() / NUM_RUN_THREADS), 3ull);
    while (sp) {
        const Job j = stack[dbuffIdx][--sp];

        const BVHNode& node = nodes[dbuffIdx][j.nodeIndex];


        // classify quickly using mask from parent
        //auto res = frustumTest_centerExtent(node.center, node.extent, frustum.planes);
        auto res = MMath::aabbClassifyWithMask(node.bounds.min, node.bounds.max, f);
        if (res.state() == /*Outside*/0)
            continue;

        if (node.depth >= targetDepth) {
            frontier[dbuffIdx].push_back({ j.nodeIndex, res.mask(), res.state()});
            continue;
        }

        if (res.state() == /*Inside*/2) {
            // whole subtree accepted: push once with state=Inside
            frontier[dbuffIdx].push_back({ j.nodeIndex, 0, /*Inside*/2 });
            continue;
        }

        // Intersect: descend
        stack[dbuffIdx][sp++] = { node.leftChild, res.mask(), /*Intersect*/1};
        stack[dbuffIdx][sp++] = { node.rightChild, res.mask(), /*Intersect*/1};
    }

    static constexpr size_t T = NUM_RUN_THREADS;
    static std::vector<TraversalResult> tlsResults[] = { std::vector<TraversalResult>(T), std::vector<TraversalResult>(T) };
    for (auto& thisResult : tlsResults[dbuffIdx])
        thisResult.clear();

    static std::atomic<uint32_t> tIndex[2];
    tIndex[dbuffIdx].store(0, std::memory_order_release);
    const auto threadsToStart = std::min(frontier[dbuffIdx].size(), T);//std::clamp( std::min(T, std::min(frontier[frameIndex].size() / T, 1ull)), 1ull, T);
    MJob::for_loop(0, frontier[dbuffIdx].size(), threadsToStart,
                    [&](const size_t i) {

                        const auto thisTIndex = tIndex[dbuffIdx].fetch_add(1, std::memory_order_acq_rel);

                        //runSems[frameIndex][thisTIndex]->acquire();

                        cullArenas[dbuffIdx][thisTIndex]->reset();
                        auto& out = tlsResults[dbuffIdx][thisTIndex];
                        out.activeOccluders.reserve(1024);
                        out.nodesCulledByFrustum = 0;
                        out.nodesCulledByOcclusion = 0;
                        out.nodesVisited = 0;
                        out.visibleEntities.reserve(4096);
                        out.visibleEntities.clear();
                        out.primitivesVisited = 0;


                        // Traverse with occlusion
                        FrameArenaVector<TraversalNode> nodeStack{ FrameArenaAllocator<TraversalNode>{cullArenas[dbuffIdx][thisTIndex]} };
                        nodeStack.reserve(256);
                        nodeStack.clear();

                        float rootMinDepth, rootMaxDepth;

                        const uint32_t thisNodeIndex = frontier[dbuffIdx][i].nodeIndex;

                        //computeNodeDepthRange(rootIndex[frameIndex], cameraPos, rootMinDepth, rootMaxDepth, frameIndex);
                        if (thisNodeIndex == 0) {
                            rootMinDepth = rootMaxDepth = 0.0f;
                        } else {

                            const BVHNode& node = nodes[dbuffIdx][thisNodeIndex];
                            const AABB& bounds = node.bounds;
                            aabbViewZRange(bounds, camWorldForward, camForwardPosDot, rootMinDepth, rootMaxDepth);
                        }



                        nodeStack.push_back({ thisNodeIndex, rootMinDepth, rootMaxDepth });

                        while (!nodeStack.empty()) {
                            const TraversalNode current = nodeStack.back();
                            nodeStack.pop_back();

                            const BVHNode& node = nodes[dbuffIdx][current.nodeIndex];
                            out.nodesVisited++;

                            // Frustum test first (cheapest)
                            if (!MMath::aabbVisible(node.bounds.min, node.bounds.max, f)) {
                                out.nodesCulledByFrustum++;
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
                                for (uint32_t j = 0; j < node.primCount; ++j) {
                                    const uint32_t primIndex = node.primIndices[j];//primitiveIndices[frameIndex][node.firstPrimitive + i];
                                    const BVHPrimitive& prim = primitives[dbuffIdx][primIndex];

                                    // Fine-grained occlusion test for individual primitives
                                    bool occluded = false;
                                    const AABB& primWorldBounds = prim.bounds;//GET_AABB(prim);//prim.bounds;
                                    if (!MMath::aabbVisible(primWorldBounds.min, primWorldBounds.max, f)) {
                                        out.nodesCulledByFrustum++;
                                        continue;
                                    }


                                    if (method != OcclusionMethod::NONE && !out.activeOccluders.empty()) {
                                        //float closest = 0.0f;
                                        BoundingVolume primBounds{ registry, prim.entity };
                                        auto primVerts = primBounds.getCoarseAABB().getVertices();
                                        const float d[8] = {
                                            glm::length2(primVerts[0] - cameraPos),
                                            glm::length2(primVerts[1] - cameraPos),
                                            glm::length2(primVerts[2] - cameraPos),
                                            glm::length2(primVerts[3] - cameraPos),
                                            glm::length2(primVerts[4] - cameraPos),
                                            glm::length2(primVerts[5] - cameraPos),
                                            glm::length2(primVerts[6] - cameraPos),
                                            glm::length2(primVerts[7] - cameraPos),
                                        };
                                        const __m256 v = _mm256_loadu_ps(d);
                                        float mn;
                                        MMath::hmin8(v, mn);

                                        occluded = isOccludedRaycast(primWorldBounds, activeOccluders, cameraPos, mn, dbuffIdx) &&
                                            !primWorldBounds.contains(cameraPos);
                                    }

                                    if (!occluded) {
                                        out.visibleEntities.push_back(prim.entity);
                                    }
                                    out.primitivesVisited++;
                                }
                            } else {
                                // Add children to stack with depth ranges
                                if (node.leftChild != 0) {
                                    float minDepth, maxDepth;



                                    //computeNodeDepthRange(node.leftChild, cameraPos, minDepth, maxDepth, frameIndex);
                                    if (node.leftChild == 0) {
                                        minDepth = maxDepth = 0.0f;
                                    } else {

                                        //const BVHNode& node = nodes[frameIndex][node.leftChild];
                                        const AABB& bounds = node.bounds;
                                        aabbViewZRange(bounds, camWorldForward, camForwardPosDot, minDepth, maxDepth);
                                    }

                                    nodeStack.push_back({ node.leftChild, minDepth, maxDepth });
                                }
                                if (node.rightChild != 0) {
                                    float minDepth, maxDepth;


                                    //computeNodeDepthRange(node.rightChild, cameraPos, minDepth, maxDepth, frameIndex);
                                    if (node.rightChild == 0) {
                                        minDepth = maxDepth = 0.0f;
                                    } else {

                                        //const BVHNode& node = nodes[frameIndex][node.rightChild];
                                        const AABB& bounds = node.bounds;
                                        aabbViewZRange(bounds, camWorldForward, camForwardPosDot, minDepth, maxDepth);
                                    }


                                    nodeStack.push_back({ node.rightChild, minDepth, maxDepth });
                                }
                            }
                        }

                        //runSems[frameIndex][thisTIndex]->release();
                    });

    result.visibleEntities.clear();
    size_t totalVis = 0;
    size_t totalActiveOcc = 0;
    for (size_t i = 0; i < threadsToStart; ++i) {       
        auto& v = tlsResults[dbuffIdx][i];
        totalVis += v.visibleEntities.size();
        totalActiveOcc += v.activeOccluders.size();
        result.nodesCulledByFrustum += v.nodesCulledByFrustum;
        result.nodesCulledByOcclusion += v.nodesCulledByOcclusion;
        result.nodesVisited += v.nodesVisited;
        result.primitivesVisited += v.primitivesVisited;
    }
    result.visibleEntities.reserve(totalVis);
    result.activeOccluders.reserve(totalActiveOcc);
    for (size_t i = 0; i < T; ++i) {
        auto& v = tlsResults[dbuffIdx][i];
        result.visibleEntities.insert(result.visibleEntities.end(), v.visibleEntities.begin(), v.visibleEntities.end());
        result.activeOccluders.insert(result.activeOccluders.end(), v.activeOccluders.begin(), v.activeOccluders.end());
    }
}

TraversalResult DualBVH::broadPhaseCollision(const uint8_t index) const {
    TraversalResult result;
    if (isEmpty(index)) return result;

    // Self-collision detection using recursive traversal
    std::function<void(uint32_t, uint32_t, uint8_t)> traverse =
        [&](const uint32_t nodeA, const uint32_t nodeB, const uint8_t frameIndex)
        {

        const BVHNode& a = nodes[frameIndex][nodeA];
        const BVHNode& b = nodes[frameIndex][nodeB];

        result.nodesVisited++;

        if (!a.bounds.intersects(b.bounds)) return;

        if (a.isLeaf() && b.isLeaf()) {
            // Test all primitive pairs
            for (uint32_t i = 0; i < a._pad0; ++i) {
                for (uint32_t j = nodeA == nodeB ? i + 1 : 0; j < b._pad0; ++j) {
                    [[maybe_unused]] uint32_t primA = 0xffffffff;// primitiveIndices[frameIndex][a.firstPrimitive + i];
                    [[maybe_unused]] uint32_t primB = 0xffffffff; // primitiveIndices[frameIndex][b.firstPrimitive + j];



                    //if (primitives[frameIndex][primA].bounds.intersects(
                    //        primitives[frameIndex][primB].bounds)) {
                    //    result.collisionPairs.emplace_back(
                    //        primitives[frameIndex][primA].entity,
                    //        primitives[frameIndex][primB].entity
                    //    );
                    //    result.primitivesVisited += 2;
                    //}
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
