#ifndef BVH_hpp
#define BVH_hpp

#include <atomic>
#include <shared_mutex>
#include <cstdint>
#include <numeric>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include "BasicTypes.hpp"
#include "ArenaAllocator.hpp"

// BVH Node - designed for cache efficiency
struct alignas(64) BVHNode
{
    AABB bounds;

    // Using bit packing for efficiency
    union
    {
        struct
        {
            uint32_t leftChild;   // Index to left child (0 = no child)
            uint32_t rightChild;  // Index to right child (0 = no child)
        };
        struct
        {
            uint32_t firstPrimitive; // For leaf nodes
            uint32_t primitiveCount; // For leaf nodes
        };
    };

    uint32_t parentIndex;     // For bottom-up updates
    uint8_t flags;            // Dirty bit, leaf bit, etc.
    uint8_t splitAxis;        // X=0, Y=1, Z=2
    uint16_t padding;

    bool isLeaf() const { return flags & 0x01; }
    bool isDirty() const { return flags & 0x02; }
    void setLeaf(bool leaf) { flags = leaf ? (flags | 0x01) : (flags & ~0x01); }
    void setDirty(bool dirty) { flags = dirty ? (flags | 0x02) : (flags & ~0x02); }
};

// Primitive reference for BVH
struct BVHPrimitive
{
    entt::entity entity;
    AABB localBounds;
    AABB worldBounds;       // Cached transformed bounds
    uint32_t frameUpdated;  // Last frame this was updated

    BVHPrimitive(entt::entity e, const AABB& bounds)
        : entity(e), localBounds(bounds), worldBounds(bounds), frameUpdated(0) {}
};

struct Frustum;

// Thread-safe BVH with dual-purpose design
class DualBVH
{
private:
    std::vector<BVHNode> nodes;
    std::vector<BVHPrimitive> primitives;
    std::vector<uint32_t> primitiveIndices;  // Leaf nodes point into this
    

    // Thread safety
    //mutable std::shared_mutex treeMutex;
    std::atomic<uint32_t> currentFrame{ 0 };
    std::atomic<uint32_t> rebuildCounter{ 0 };
    std::atomic<uint32_t> dirtyCount{ 0 };

    // Entity to primitive mapping for fast lookups
    std::unordered_map<entt::entity, uint32_t> entityToPrimitive;

    // Configuration
    static constexpr uint32_t MAX_LEAF_PRIMITIVES = 4;
    static constexpr float REBUILD_THRESHOLD = 0.3f; // 30% of objects moved

    uint32_t rootIndex = 0;
    uint32_t nodeCount = 0;

public:
    struct BuildSettings
    {
        uint32_t maxLeafPrimitives = MAX_LEAF_PRIMITIVES;
        bool useMedianSplit = false;  // false = SAH, true = median
        float rebuildThreshold = REBUILD_THRESHOLD;
    };

    struct TraversalResult
    {
        std::vector<entt::entity> visibleEntities;
        std::vector<entt::entity> activeOccluders;
        std::vector<std::pair<entt::entity, entt::entity>> collisionPairs;
        uint32_t nodesVisited = 0;
        uint32_t primitivesVisited = 0;
        uint32_t nodesCulledByFrustum = 0;
        uint32_t nodesCulledByOcclusion = 0;

        void clear() {
            visibleEntities.clear();
            activeOccluders.clear();
            collisionPairs.clear();
            nodesVisited = 0;
            primitivesVisited = 0;
            nodesCulledByFrustum = 0;
            nodesCulledByOcclusion = 0;
        }
    };

    // Occlusion data
    struct OccluderData
    {
        entt::entity entity;
        AABB bounds;
        //float volume;
        float depth;  // Distance from camera
        bool isOccluder;  // Can this entity occlude others?

        //OccluderData(entt::entity e, const AABB& b, float v, float d, bool occluder = false)
        //    : entity(e), bounds(b), depth(d), isOccluder(occluder), volume( v ) {}
        OccluderData(entt::entity e, const AABB& b,  float d, bool occluder = false)
            : entity(e), bounds(b), depth(d), isOccluder(occluder) {}
    };

    enum class OcclusionMethod
    {
        NONE,
            SIMPLE_DEPTH,     // Simple front-to-back + depth test
            HIERARCHICAL_Z,   // Hierarchical Z-buffer (more advanced)
            PORTAL_ZONES      // For indoor scenes with portals
    };


    BuildSettings settings{};
    std::vector<uint32_t> occluderIndices;
private:
    // Building methods
    uint32_t buildRecursive(std::vector<uint32_t>& primitiveIds, uint32_t depth, uint32_t parent);

    uint32_t findBestSplit(const std::vector<uint32_t>& primitiveIds, int& bestAxis, float& bestPos);
    AABB computeBounds(const std::vector<uint32_t>& primitiveIds) {
        if (primitiveIds.empty()) {
            return AABB();
        }

        AABB bounds = primitives[primitiveIds[0]].worldBounds;
        for (size_t i = 1; i < primitiveIds.size(); ++i) {
            bounds = bounds.merge(primitives[primitiveIds[i]].worldBounds);
        }

        return bounds;
    }

    // Update methods
    void updatePrimitive(uint32_t primIndex, const glm::mat4x4& transform) {
        BVHPrimitive& prim = primitives[primIndex];
        AABB newWorldBounds = prim.localBounds.transformed_noPerspective(transform);

        // Check if bounds actually changed significantly
        if (!boundsChanged(prim.worldBounds, newWorldBounds)) {
            prim.frameUpdated = currentFrame;
            return;
        }

        prim.worldBounds = newWorldBounds;
        prim.frameUpdated = currentFrame;

        // Mark nodes as dirty up the hierarchy
        // Note: This is simplified - in practice you'd want to track which 
        // leaf each primitive belongs to for faster updates
        dirtyCount++;
    }
    void refitBottomUp(uint32_t nodeIndex) {
        if (nodeIndex == 0) return;

        BVHNode& node = nodes[nodeIndex];

        if (node.isLeaf()) {
            // Recompute bounds from primitives
            std::vector<uint32_t> primitiveIds;
            for (uint32_t i = 0; i < node.primitiveCount; ++i) {
                primitiveIds.push_back(primitiveIndices[node.firstPrimitive + i]);
            }
            node.bounds = computeBounds(primitiveIds);
        } else {
            // Recompute from children
            AABB newBounds;
            if (node.leftChild != 0) {
                newBounds = newBounds.merge(nodes[node.leftChild].bounds);
            }
            if (node.rightChild != 0) {
                newBounds = newBounds.merge(nodes[node.rightChild].bounds);
            }
            node.bounds = newBounds;
        }

        node.setDirty(false);

        // Continue up the tree
        if (node.parentIndex != 0) {
            refitBottomUp(node.parentIndex);
        }
    }

    bool needsRebuild() const {
        return dirtyCount > (primitives.size() * settings.rebuildThreshold);
    }

    // Helper methods
    bool boundsChanged(const AABB& oldBounds, const AABB& newBounds, float threshold = 0.01f) const {
        glm::vec3 oldSize = oldBounds.size();
        glm::vec3 newSize = newBounds.size();
        glm::vec3 oldCenter = oldBounds.center();
        glm::vec3 newCenter = newBounds.center();

        return glm::length(oldSize - newSize) > threshold ||
            glm::length(oldCenter - newCenter) > threshold;
    }

    // Occlusion culling helper methods
    void collectOccluders(const glm::vec3& cameraPos,
                          const Frustum& frustum,
                          std::vector<OccluderData>& occluders) const;

    bool isOccludedRaycast(const AABB& bounds, const AABB& occluder,
                                const glm::vec3& cameraPos) const {

       auto verts = occluder.getVertices();
       for (auto& vert : verts) {
           auto dir = vert - cameraPos;
           Ray r{ cameraPos, dir };
           if (bounds.intersects(r))
               return false;
       }
  
        return true;
    }

        // Enhanced ray-AABB intersection that returns distance
    bool rayAABBIntersectWithDistance(const glm::vec3& origin, const glm::vec3& direction,
                                      const AABB& aabb, float& tNear, float& tFar) const {
        glm::vec3 invDir = 1.0f / direction;
        glm::vec3 t1 = (aabb.min - origin) * invDir;
        glm::vec3 t2 = (aabb.max - origin) * invDir;

        glm::vec3 tmin = glm::min(t1, t2);
        glm::vec3 tmax = glm::max(t1, t2);

        tNear = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
        tFar = glm::min(glm::min(tmax.x, tmax.y), tmax.z);

        return tNear <= tFar && tFar >= 0.0f;
    }

    bool isOccludedRaycast(const AABB& bounds, const std::vector<AABB>& occluders,
                    const glm::vec3& cameraPos, float objectDepth) const {

        // Ray-based occlusion test - much more accurate!
        auto objectCorners = bounds.getVertices();

        // Test rays from camera to each corner of the object
        int visibleCorners = 0;

        for (int i = 0; i < 8; ++i) {
            glm::vec3 rayDir = glm::normalize(objectCorners[i] - cameraPos);
            float rayLength = glm::length2(objectCorners[i] - cameraPos);

            bool rayBlocked = false;

            // Test this ray against all occluders
            for (const auto& occluder : occluders) {
                glm::vec3 occluderCenter = occluder.center();
                float occluderDepth = glm::length2(occluderCenter - cameraPos);

                // Only test occluders that are closer than the object
                if (occluderDepth >= objectDepth - 0.1f) continue;

                // Ray-AABB intersection test
                float tNear, tFar;
                if (rayAABBIntersectWithDistance(cameraPos, rayDir, occluder, tNear, tFar)) {
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

    bool isOccluded(const AABB& bounds, const std::vector<AABB>& occluders,
                    const glm::vec3& cameraPos, float objectDepth) const {

        glm::vec3 objectCenter = bounds.center();
        glm::vec3 viewDir = glm::normalize(objectCenter - cameraPos);

        // Simple occlusion test: check if any occluder blocks the view to this object
        for (const auto& occluder : occluders) {
            glm::vec3 occluderCenter = occluder.center();
            float occluderDepth = glm::length(occluderCenter - cameraPos);

            // Only test occluders that are closer
            if (occluderDepth >= objectDepth - 0.5f)
                continue;

            // Simple shadow volume test
            if (isShadowedBy(bounds, occluder, cameraPos)) {
                return true;
            }
        }

        return false;
    }

    bool isShadowedBy(const AABB& object, const AABB& occluder, const glm::vec3& cameraPos) const {
        // Simplified shadow volume test
        // In practice, you'd want more sophisticated occlusion testing

        glm::vec3 objCenter = object.center();
        glm::vec3 occCenter = occluder.center();
        glm::vec3 objSize = object.size();
        glm::vec3 occSize = occluder.size();

        // Check if object is behind occluder from camera's perspective
        glm::vec3 camToOcc = occCenter - cameraPos;
        glm::vec3 camToObj = objCenter - cameraPos;

        if (glm::dot(camToObj, camToOcc) <= 0) return false; // Object not behind occluder

        // Project occluder bounds onto the plane containing the object
        float t = glm::length(camToObj) / glm::length(camToOcc);
        glm::vec3 projectedOccCenter = cameraPos + camToOcc * t;
        glm::vec3 projectedOccSize = occSize * t; // Approximate projection scaling

        // Check if projected occluder overlaps with object (simplified 2D test)
        AABB projectedOccluder(projectedOccCenter - projectedOccSize * 0.5f,
                               projectedOccCenter + projectedOccSize * 0.5f);

        return projectedOccluder.intersects(object);
    }

    void computeNodeDepthRange(uint32_t nodeIndex, const glm::vec3& cameraPos,
                               float& minDepth, float& maxDepth) const {
        if (nodeIndex == 0) {
            minDepth = maxDepth = 0.0f;
            return;
        }

        const BVHNode& node = nodes[nodeIndex];
        const AABB& bounds = node.bounds;

        // Compute distance to all 8 corners of the AABB
        auto corners = bounds.getVertices();

        minDepth = FLT_MAX;
        maxDepth = -FLT_MAX;

        for (int i = 0; i < 8; ++i) {
            float depth = glm::length2(corners[i] - cameraPos);
            minDepth = std::min(minDepth, depth);
            maxDepth = std::max(maxDepth, depth);
        }
    }

public:
    explicit DualBVH(const BuildSettings& settings = {}) : settings(settings) {
        nodes.reserve(1024);  // Pre-allocate for better performance
        primitives.reserve(512);
        primitiveIndices.reserve(512);
    }

    // Construction
    void build(ArenaRegistry& registry);
    void rebuild(ArenaRegistry& registry) {
        rebuildCounter++;
        build(registry);
    }
    void incrementalUpdate(ArenaRegistry& registry);


    // Queries - these can run concurrently
    void frustumCull(DualBVH::TraversalResult& result, const Frustum& f) const;

    void frustumCullWithOcclusion(DualBVH::TraversalResult& result, const Frustum& f,
                                             const glm::vec3& cameraPos,
                                             OcclusionMethod method) const;

    TraversalResult broadPhaseCollision() const;

    std::vector<entt::entity> raycast(const Ray& r) const {
        std::vector<entt::entity> hits;
        if (isEmpty()) return hits;

        std::vector<uint32_t> nodeStack;
        nodeStack.reserve(64);
        nodeStack.push_back(rootIndex);

        while (!nodeStack.empty()) {
            uint32_t nodeIndex = nodeStack.back();
            nodeStack.pop_back();

            const BVHNode& node = nodes[nodeIndex];

            // Ray-AABB intersection test
            if (!node.bounds.intersects(r)) {
                continue;
            }

            if (node.isLeaf()) {
                for (uint32_t i = 0; i < node.primitiveCount; ++i) {
                    uint32_t primIndex = primitiveIndices[node.firstPrimitive + i];
                    if (primitives[primIndex].worldBounds.intersects(r)) {
                        hits.push_back(primitives[primIndex].entity);
                    }
                }
            } else {
                if (node.leftChild != 0) nodeStack.push_back(node.leftChild);
                if (node.rightChild != 0) nodeStack.push_back(node.rightChild);
            }
        }

        return hits;
    }
    std::vector<entt::entity> raycast(const glm::vec3& origin,
                                      const glm::vec3& direction) const {
        return raycast(Ray{ origin, direction });
    }


    // Management
    void addPrimitive(entt::entity entity, const AABB& bounds) {
        if (entityToPrimitive.find(entity) != entityToPrimitive.end()) {
            return; // Already exists
        }

        uint32_t primIndex = primitives.size();
        primitives.emplace_back(entity, bounds);
        primitives.back().worldBounds = bounds;  // Will be updated with transform
        primitives.back().frameUpdated = currentFrame;

        entityToPrimitive[entity] = primIndex;

        // Mark for rebuild (simple approach - could be optimized with insertion)
        dirtyCount = primitives.size();
    }
    void removePrimitive(entt::entity entity) {
        auto it = entityToPrimitive.find(entity);
        if (it == entityToPrimitive.end()) return;

        uint32_t primIndex = it->second;

        // Swap with last element
        if (primIndex != primitives.size() - 1) {
            primitives[primIndex] = primitives.back();
            entityToPrimitive[primitives[primIndex].entity] = primIndex;
        }

        primitives.pop_back();
        entityToPrimitive.erase(it);

        // Mark for rebuild
        dirtyCount = primitives.size();
    }
    void markDirty(entt::entity entity) {
        auto it = entityToPrimitive.find(entity);
        if (it != entityToPrimitive.end()) {
            uint32_t primIndex = it->second;
            if (primitives[primIndex].frameUpdated < currentFrame) {
                primitives[primIndex].frameUpdated = currentFrame - 1; // Force update
                dirtyCount++;
            }
        }
    }
    void nextFrame() {
        currentFrame++;
        dirtyCount = 0;
    }

    // Thread-safe accessors
    uint32_t getNodeCount() const { return nodeCount; }
    uint32_t getPrimitiveCount() const { return primitives.size(); }
    bool isEmpty() const { return primitives.empty(); }

    // Debug/profiling methods
    void printStats() const {
        if (isEmpty()) {
            std::cout << "BVH is empty\n";
            return;
        }

        std::cout << "BVH Stats:\n";
        std::cout << "  Nodes: " << nodeCount << "\n";
        std::cout << "  Primitives: " << primitives.size() << "\n";
        std::cout << "  Rebuild count: " << rebuildCounter.load() << "\n";
        std::cout << "  Current frame: " << currentFrame.load() << "\n";

        // Calculate tree depth
        uint32_t maxDepth = 0;
        std::function<void(uint32_t, uint32_t)> calculateDepth =
            [&](uint32_t nodeIndex, uint32_t depth) {
            if (nodeIndex == 0) return;
            maxDepth = std::max(maxDepth, depth);

            const BVHNode& node = nodes[nodeIndex];
            if (!node.isLeaf()) {
                calculateDepth(node.leftChild, depth + 1);
                calculateDepth(node.rightChild, depth + 1);
            }
            };

        calculateDepth(rootIndex, 0);
        std::cout << "  Max depth: " << maxDepth << "\n";
    }

    // Validate tree structure (debug)
    bool validateTree() const {
        if (isEmpty()) return true;

        std::function<bool(uint32_t)> validate = [&](uint32_t nodeIndex) -> bool {
            if (nodeIndex == 0 || nodeIndex > nodeCount) return false;

            const BVHNode& node = nodes[nodeIndex];

            if (node.isLeaf()) {
                // Validate leaf node
                if (node.primitiveCount == 0) return false;
                if (node.firstPrimitive + node.primitiveCount > primitiveIndices.size()) return false;

                // Check bounds contain all primitives
                for (uint32_t i = 0; i < node.primitiveCount; ++i) {
                    uint32_t primIndex = primitiveIndices[node.firstPrimitive + i];
                    if (primIndex >= primitives.size()) return false;

                    // Note: This is a simplified check - in practice you'd want 
                    // more sophisticated bounds validation
                }
            } else {
                // Validate internal node
                if (node.leftChild == 0 && node.rightChild == 0) return false;

                if (node.leftChild != 0 && !validate(node.leftChild)) return false;
                if (node.rightChild != 0 && !validate(node.rightChild)) return false;
            }

            return true;
            };

        return validate(rootIndex);
    }

};

// Usage example system
class BVHSystem
{
private:
    DualBVH bvh;
    uint32_t lastUpdateFrame = 0;

public:
    // Called once per frame in main thread
    void updateBVH(ArenaRegistry& registry, uint32_t currentFrame) {
        if (currentFrame != lastUpdateFrame) {
            bvh.incrementalUpdate(registry);
            bvh.nextFrame();
            lastUpdateFrame = currentFrame;
        }
    }

    // Called from render thread
    auto performFrustumCulling(DualBVH::TraversalResult& result, const Frustum& f) const {
        return bvh.frustumCull(result, f);
    }

    // Called from render thread with occlusion culling
    auto performFrustumCullingWithOcclusion(DualBVH::TraversalResult& result,
                                            const Frustum& f,
                                            const glm::vec3& cameraPos,
                                            DualBVH::OcclusionMethod method = DualBVH::OcclusionMethod::SIMPLE_DEPTH) const {
        return bvh.frustumCullWithOcclusion(result, f, cameraPos, method);
    }

    // Called from physics thread (can be concurrent with frustum culling)
    auto performBroadPhase() const {
        return bvh.broadPhaseCollision();
    }

    // Add/remove entities
    void addEntity(entt::entity entity, const AABB& bounds) {
        bvh.addPrimitive(entity, bounds);
    }

    void removeEntity(entt::entity entity) {
        bvh.removePrimitive(entity);
    }

    void markEntityDirty(entt::entity entity) {
        bvh.markDirty(entity);
    }

    // Query methods
    std::vector<entt::entity> raycast(const glm::vec3& origin, const glm::vec3& direction) const {
        return bvh.raycast(origin, direction);
    }

    // Debug/profiling
    void printStats() const { bvh.printStats(); }
    bool validate() const { return bvh.validateTree(); }

    // Force rebuild (useful for debugging or after major scene changes)
    void forceRebuild(ArenaRegistry& registry) {
        bvh.rebuild(registry);
    }
};

#endif