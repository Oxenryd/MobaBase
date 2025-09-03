#ifndef BVH_hpp
#define BVH_hpp

#include <atomic>
#include <shared_mutex>
#include <numeric>
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include <semaphore>
#include <barrier>

#include "EnabledTag.hpp"
#include "BasicTypes.hpp"
#include "ArenaAllocator.hpp"
#include "BoundingVolume.hpp"
#include "Camera.hpp"

#define GET_AABB(prim) Engine::getInstance()->getScene(static_cast<size_t>((prim).sceneIndex))->boundingSystem().aabbs()[(prim).aabbIndex];

using TransformGroup =
decltype(std::declval<ArenaRegistry&>()
         .group<TransformComponent, BoundingVolumeComponent, EnabledTag>());

// BVH Node - designed for cache efficiency
struct BVHNode //alignas(64)
{
    //AABB bounds{};

    glm::vec3 center{};
    glm::vec3 extent{};

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
            uint32_t _pad0; // For leaf nodes
        };
    };

    uint32_t parentIndex{ UINT32_INVALID };     // For bottom-up updates
    uint8_t flags{ UINT8_INVALID };            // Dirty bit, leaf bit, etc.
    uint8_t splitAxis{ UINT8_INVALID };        // X=0, Y=1, Z=2
    uint8_t primCount{ UINT8_INVALID };
    uint8_t depth{ 0 };
    std::array<uint32_t, 6> primIndices;

    bool isLeaf() const { return flags & 0x01; }
    bool isDirty() const { return flags & 0x02; }
    void setLeaf(bool leaf) { flags = leaf ? (flags | 0x01) : (flags & ~0x01); }
    void setDirty(bool dirty) { flags = dirty ? (flags | 0x02) : (flags & ~0x02); }

    INLINE glm::vec3 min() const {
        return center - 0.5f * extent;
    }
    INLINE glm::vec3 max() const {
        return center + 0.5f * extent;
    }
    INLINE AABB bounds() const {
        return AABB{ min(), max() };
    }
};

// Primitive reference for BVH
struct BVHPrimitive
{
    entt::entity entity;
    AABB bounds;
    //uint32_t aabbIndex;
    //uint16_t sceneIndex;
    //AABB bounds;
    //uint32_t frameUpdated;  // Last frame this was updated
    //BoundingVolume bounds;
    //uint32_t local_BIndex;
    //uint32_t world_BIndex;
    //AABB localBounds;
    //AABB worldBounds;       // Cached transformed bounds
    
    BVHPrimitive() : 
        entity{ entt::null }, bounds{}//aabbIndex{UINT32_INVALID}, sceneIndex{UINT16_INVALID}
    {}
    BVHPrimitive(const BVHPrimitive& other) :
        entity{ other.entity }, bounds{ other.bounds }//sceneIndex{other.sceneIndex}, aabbIndex{other.aabbIndex}
    {}
    BVHPrimitive& operator=(const BVHPrimitive& rhs) {
        if (this == &rhs) return *this;

        entity = rhs.entity;
        bounds = rhs.bounds;
        //sceneIndex = rhs.sceneIndex;
        //aabbIndex = rhs.aabbIndex;

        return *this;
    }

    BVHPrimitive& operator=(BVHPrimitive&& rhs) noexcept {
        if (this == &rhs) return *this;

        rhs.entity = entt::null;

        entity = rhs.entity;
        bounds = rhs.bounds;
        //sceneIndex = rhs.sceneIndex;
        //aabbIndex = rhs.aabbIndex;

        return *this;
    }

    //BVHPrimitive(entt::entity e, uint32_t dataIndex, uint16_t scene) :
    //    entity{ e }, sceneIndex{ scene }, aabbIndex{ dataIndex } //frameUpdated{ UINT32_INVALID } 
    //{}
    BVHPrimitive(entt::entity e, const AABB& aabb) :
        entity{e}, bounds{aabb}
    {}

    //BVHPrimitive(entt::entity e, const AABB& bounds)
    //    : entity(e), localBounds(bounds), worldBounds(bounds), frameUpdated(0) {}
};

struct Frustum;

// Thread-safe BVH with dual-purpose design


class DualBVH
{
private:

    struct TestResult { 
        TestResult(uint8_t mask, uint8_t state) :
            state{state}, mask{mask} {}
        union
        {
            uint8_t raw;
            struct
            {
                uint8_t state : 2;
                uint8_t mask : 6;
            };
        };        
    };

    inline TestResult frustumTest_centerExtent(const glm::vec3& c, const glm::vec3& e,
                                               const FrustumPlane* F) {
        // For each plane: s = dot(n, c) + d; r = dot(|n|, e).
        // Outside if s + r < 0, Inside if s - r >= 0, else Intersect.
        uint8_t mask = 0;
        bool anyIntersect = false;

        for (int i = 0; i < 6; ++i) {
            const glm::vec3 n = glm::vec3(F[i].vec);
            const float d = F[i].vec.w;
            const float s = glm::dot(n, c) + d;
            const float r = glm::dot(glm::abs(n), e);
            if (s + r < 0.0f)
                return { 0, /*Outside*/0 };
            if (s - r < 0.0f) {
                anyIntersect = true; mask |= (1u << i);
            }
        }

        return { mask, anyIntersect ? /*Intersect*/(uint8_t)1 : /*Inside*/(uint8_t)2 };
    }
    inline TestResult frustumTest_centerExtent_masked(const glm::vec3& c, const glm::vec3& e,
                                               const FrustumPlane* F, uint8_t planeMask) {
        // For each plane: s = dot(n, c) + d; r = dot(|n|, e).
        // Outside if s + r < 0, Inside if s - r >= 0, else Intersect.
        uint8_t mask = 0;
        bool anyIntersect = false;

        for (int i = 0; i < 6; ++i) {
            const glm::vec3 n = glm::vec3(F[i].vec);
            const float d = F[i].vec.w;
            const float s = glm::dot(n, c) + d;
            const float r = glm::dot(glm::abs(n), e);
            if (s + r < 0.0f) return { 0, /*Outside*/0 };
            if (s - r < 0.0f) { anyIntersect = true; mask |= (1u << i); }
        }
        return { mask, anyIntersect ? /*Intersect*/(uint8_t)1 : /*Inside*/(uint8_t)2 };
    }




    INLINE void _tSafe_insertionSort(uint32_t* primitiveIds, uint32_t primCount, int bestAxis, uint8_t index);
    static constexpr const unsigned int NUM_BUILD_THREADS = 6;
    static constexpr const unsigned int NUM_RUN_THREADS = 16;
    std::array<FrameArena*, NUM_RUN_THREADS + 1> cullArenas[2]{ nullptr, nullptr };
    std::array<std::binary_semaphore*, NUM_RUN_THREADS> runSems[2]{ nullptr, nullptr };
    std::atomic<TransformGroup*> m_groupPtr = nullptr;
    ArenaRegistry& m_reg;

public:
    std::array<std::vector<BVHNode>, 2> nodes;
    std::array<std::vector<BVHPrimitive>, 2> primitives;
    std::array<std::vector<uint32_t>, 2> primitiveIndices;  // Leaf nodes point into this

    // Thread safety
    //mutable std::shared_mutex treeMutex;
    //std::atomic<uint32_t> currentFrame{ 0 };
    //std::atomic<uint32_t> rebuildCounter{ 0 };
    //std::atomic<uint32_t> dirtyCount{ 0 };

    // Entity to primitive mapping for fast lookups
    std::array<std::unordered_map<entt::entity, uint32_t>, 2> entityToPrimitive;

    // Configuration
    static constexpr uint32_t MAX_DEPTH = 32;
    static constexpr uint32_t MAX_LEAF_PRIMITIVES = 4;
    static constexpr float REBUILD_THRESHOLD = 0.3f; // 30% of objects moved

    std::array<std::atomic<uint32_t>, 2> rootIndex;
    //std::array<uint32_t, 2> nodeCount;
    std::atomic<uint32_t> nodeCount[2];
    std::atomic<uint32_t> indicesCount[2];
                                     
    struct alignas(64) WorkerPkg
    {
        ~WorkerPkg() = default;
        WorkerPkg() = default;
        WorkerPkg(Arena* arena) :
            primitiveIds{ArenaAllocator<uint32_t>{arena}} {}
        WorkerPkg(const WorkerPkg& other) = default;
        WorkerPkg& operator=(WorkerPkg& rhs) {
            depth = rhs.depth;
            parent = rhs.parent;
            index = rhs.index;
            primCount = rhs.primCount;
            primitiveIds = std::move(rhs.primitiveIds);

            return *this;
        }
        WorkerPkg(WorkerPkg&& other) = default;
        WorkerPkg& operator=(WorkerPkg&& rhs) = default;

        ArenaVector<uint32_t> primitiveIds;
        uint32_t primCount{ UINT32_INVALID };
        uint8_t depth{ UINT8_INVALID };
        uint32_t parent{ UINT32_INVALID };
        uint8_t index{ UINT8_INVALID };

    };
    std::atomic<bool> workersRunning = true;
    //std::atomic<bool> hasBuiltPrimitivesOnce[2] = {false, false};
    std::atomic<uint8_t> nextThId;
    std::array<std::thread*, NUM_BUILD_THREADS> workers;
    std::array<WorkerPkg*, NUM_BUILD_THREADS> workerPkgs;
    std::array<std::atomic<bool>, NUM_BUILD_THREADS> workPkgCondition;
    std::array<std::binary_semaphore*, NUM_BUILD_THREADS> startSemas;
    std::binary_semaphore primitivesStartSema{ 0 };
    std::binary_semaphore primitivesLock{ 1 };
    std::thread* rebuildThreads = nullptr;
    std::array<std::binary_semaphore*, NUM_BUILD_THREADS> doneSemas;
    //std::array<std::vector<uint32_t>, NUM_BUILD_THREADS> workerPrimsTemp;
    std::array<std::atomic<uint32_t>, NUM_BUILD_THREADS> workerResults;

    static void _updatePrimitives(DualBVH* _this);
    static void _recursiveWorker(DualBVH* _this, uint8_t threadId);

    uint8_t _getNextThreadId() {
        return nextThId.fetch_add(1, std::memory_order_seq_cst);
    }


    std::vector<entt::entity> alwaysVisible[2];
    struct BuildSettings
    {
        uint32_t maxDepth = MAX_DEPTH;
        uint32_t maxLeafPrimitives = MAX_LEAF_PRIMITIVES;
        bool useMedianSplit = true;  // false = SAH, true = median
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
        //AABB bounds;
        uint32_t primIndex;
        //float volume;
        float depth;  // Distance from camera
        uint32_t cornersOffset;
        //bool isOccluder;  // Can this entity occlude others?
        OccluderData() = default;
        OccluderData(const OccluderData& other) = default;
        OccluderData& operator=(const OccluderData& rhs) = default;
        //OccluderData(entt::entity e, const AABB& b, float v, float d, bool occluder = false)
        //    : entity(e), bounds(b), depth(d), isOccluder(occluder), volume( v ) {}
        //OccluderData(entt::entity e, const AABB& b, float d, uint32_t cornerOffset, uint32_t primitiveIndex)
        //    : entity(e), bounds(b), depth(d), cornersOffset(cornerOffset), primIndex(primitiveIndex) {} //isOccluder(occluder) {}
        OccluderData(entt::entity e, float d, uint32_t cornerOffset, uint32_t primitiveIndex)
            : entity(e), depth(d), cornersOffset(cornerOffset), primIndex(primitiveIndex) {} //isOccluder(occluder) {}
    };

    enum class OcclusionMethod
    {
        NONE,
        SIMPLE_DEPTH,     // Simple front-to-back + depth test
        HIERARCHICAL_Z,   // Hierarchical Z-buffer (more advanced)
        PORTAL_ZONES      // For indoor scenes with portals
    };

    struct TraversalNode
    {
        uint32_t nodeIndex;
        float minDepth;
        float maxDepth;
    };

    void setPendingUpdate(uint8_t& outIndex) {
        if (_threadDone.try_acquire()) {
            _frameArena->reset();
            nextThId.store(0, std::memory_order_release);
            outIndex = _getIndexToUseThisFrame();
            _threadStart.release();
        }
    }
    bool hasValidHierarchy() const {
        return _threadIndexToUse.load() != UINT8_INVALID && curFrameIndex != UINT8_INVALID;
    }

    BuildSettings settings{};
    using OccIndexCornerIndexDepthTuple = std::tuple<uint32_t, uint32_t, float>;
    std::array<std::vector<OccIndexCornerIndexDepthTuple>, 2> occluderIndices;
    std::array<std::vector<glm::vec3>, 2> occluderCorners;

    //bool threadRunning() const { return _threadRunning; }

    uint8_t curFrameIndex = UINT8_INVALID;
    //std::array<std::vector<TraversalNode>, 2> nodeStack;
    std::thread* rebuildThread;
    std::atomic<uint8_t> _threadIndexToUse = UINT8_INVALID;
    std::binary_semaphore _threadStart{ 0 };
    std::binary_semaphore _threadDone{ 1 };
    Arena* _frameArena = nullptr;
    //bool _threadRunning = true;

    // Building methods
    uint8_t _getIndexToUseThisFrame();
    static void _buildThreadMethod(DualBVH* _this, ArenaRegistry& registry);
    void rebuildPrimitives(ArenaRegistry& registry, uint8_t index);
    uint32_t buildRecursive(uint32_t* primitiveIds, uint32_t primCount, uint8_t depth, uint32_t parent, uint8_t index);

    uint32_t findBestSplit(const uint32_t* primitiveIds, uint32_t primCount, int& bestAxis, float& bestPos, uint8_t index);
    AABB computeBounds(const uint32_t* primitiveIds, uint32_t primCount, uint8_t index);

    // Update methods
    void updatePrimitive(uint32_t primIndex, const glm::mat4x4& transform, uint8_t index) {
        //BVHPrimitive& prim = primitives[index][primIndex];
        //AABB primWorldBounds = prim.bounds;
        //AABB newWorldBounds = prim.bounds.getCoarseAABB_local().transformed_noPerspective(transform);

        //// Check if bounds actually changed significantly
        //if (!boundsChanged(primWorldBounds, newWorldBounds)) {
        //    prim.frameUpdated = currentFrame;
        //    return;
        //}

        //prim.bounds.setCoarseAABB(newWorldBounds);
        //prim.frameUpdated = currentFrame;

        // Mark nodes as dirty up the hierarchy
        // Note: This is simplified - in practice you'd want to track which 
        // leaf each primitive belongs to for faster updates
        //dirtyCount++;
    }
    //void refitBottomUp(uint32_t nodeIndex, uint8_t index) {
    //    if (nodeIndex == 0) return;

    //    BVHNode& node = nodes[index][nodeIndex];

    //    if (node.isLeaf()) {
    //        // Recompute bounds from primitives
    //        std::vector<uint32_t> primitiveIds;
    //        for (uint32_t i = 0; i < node._pad0; ++i) {
    //            primitiveIds.push_back(primitiveIndices[index][node.firstPrimitive + i]);
    //        }
    //        node.bounds = computeBounds(primitiveIds, index);
    //    } else {
    //        // Recompute from children
    //        AABB newBounds;
    //        if (node.leftChild != 0) {
    //            newBounds = newBounds.merge(nodes[index][node.leftChild].bounds);
    //        }
    //        if (node.rightChild != 0) {
    //            newBounds = newBounds.merge(nodes[index][node.rightChild].bounds);
    //        }
    //        node.bounds = newBounds;
    //    }

    //    node.setDirty(false);

    //    // Continue up the tree
    //    if (node.parentIndex != 0) {
    //        refitBottomUp(node.parentIndex, index);
    //    }
    //}

    //bool needsRebuild() const {
    //    return dirtyCount > (primitives.size() * settings.rebuildThreshold);
    //}

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

    bool isOccludedRaycast(const AABB& bounds, const std::vector<OccIndexCornerIndexDepthTuple>& occluders,
                           const glm::vec3& cameraPos, float objectDepth, uint8_t index) const;

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

    //void computeNodeDepthRange(uint32_t nodeIndex, const glm::vec3& cameraPos,
    //                           float& minDepth, float& maxDepth, uint8_t index) {
    //    if (nodeIndex == 0) {
    //        minDepth = maxDepth = 0.0f;
    //        return;
    //    }

    //    const BVHNode& node = nodes[index][nodeIndex];
    //    const AABB& bounds = node.bounds;

    //    // Compute distance to all 8 corners of the AABB
    //    auto corners = bounds.getVertices();

    //    minDepth = FLT_MAX;
    //    maxDepth = -FLT_MAX;

    //    for (int i = 0; i < 8; ++i) {
    //        float depth = glm::length2(corners[i] - cameraPos);
    //        minDepth = std::min(minDepth, depth);
    //        maxDepth = std::max(maxDepth, depth);
    //    }
    //}


    ~DualBVH() {
        workersRunning.store(false, std::memory_order_release);
        _threadStart.release();
        rebuildThread->join();
        delete rebuildThread;

        primitivesStartSema.release();

        delete _frameArena;
        _frameArena = nullptr;

        workersRunning = false;
        for (size_t i = 0; i < NUM_BUILD_THREADS; ++i) {
            startSemas[i]->release();
            workers[i]->join();
            delete workers[i];
            workers[i] = nullptr;

            delete startSemas[i];
            startSemas[i] = nullptr;
            delete doneSemas[i];
            doneSemas[i] = nullptr;

            //if (workerPkgs[i])
            //    delete workerPkgs[i];
            //workerPkgs[i] = nullptr;
        }

        for (size_t i = 0; i < NUM_RUN_THREADS; ++i) {
            delete cullArenas[0][i];
            cullArenas[0][i] = nullptr;
            delete cullArenas[1][i];
            cullArenas[1][i] = nullptr;
            
            delete runSems[0][i];
            runSems[0][i] = nullptr;
            delete runSems[1][i];
            runSems[1][i] = nullptr;
        }
        delete cullArenas[0][NUM_RUN_THREADS];
        cullArenas[1][NUM_RUN_THREADS] = nullptr;

        rebuildThreads->join();
        //rebuildThreads[1]->join();
        delete rebuildThreads;
        //delete rebuildThreads[1];
        rebuildThreads = nullptr;
        //rebuildThreads[1] = nullptr;
    }
    explicit DualBVH(ArenaRegistry& registry, const BuildSettings& settings = {}) :
        settings(settings), m_reg{registry}
        //, primitivesStartSema{ std::binary_semaphore{0}, std::binary_semaphore{0} },
        //primitivesLock{ std::binary_semaphore{1}, std::binary_semaphore{1} }
    {
        for (size_t i = 0; i < 2; ++i) {
            nodes[i].reserve(1024);  // Pre-allocate for better performance
            primitives[i].reserve(512);
            occluderIndices[i].reserve(256);
            occluderCorners[i].reserve(1024);
            rootIndex[i].store(0);
            nodeCount[i].store(0);
        }

        _frameArena = new Arena(32_MB);

        //_threadRunning = true;
        rebuildThread = new std::thread{
            DualBVH::_buildThreadMethod,
            this,
            std::ref(registry) };

        workersRunning = true;
        for (size_t i = 0; i < NUM_BUILD_THREADS; ++i) {
            startSemas[i] = new std::binary_semaphore{ 0 };
            doneSemas[i] = new std::binary_semaphore{ 0 };
            workers[i] = new std::thread{ DualBVH::_recursiveWorker, this, static_cast<uint8_t>(i) };
            workerResults[i] = 0;
            workerPkgs[i] = nullptr;

            workPkgCondition[i] = false;
        }

        for (size_t i = 0; i < NUM_RUN_THREADS; ++i) {
            cullArenas[0][i] = new FrameArena{ 768_KB };
            cullArenas[1][i] = new FrameArena{ 768_KB };
            runSems[0][i] = new std::binary_semaphore{ 1 };
            runSems[1][i] = new std::binary_semaphore{ 1 };
        }
        cullArenas[0][NUM_RUN_THREADS] = new FrameArena{ 768_KB };
        cullArenas[1][NUM_RUN_THREADS] = new FrameArena{ 768_KB };

        rebuildThreads = new std::thread{DualBVH::_updatePrimitives, this};
        //rebuildThreads[1] = new std::thread{ DualBVH::_updatePrimitives, this, 1u };
    }

    void startRebuild(ArenaRegistry& registry) {

        primitivesStartSema.release();
    }

    // Construction
    void buildNodes(ArenaRegistry& registry, uint8_t index);
    //void rebuild(ArenaRegistry& registry) {
    //    rebuildCounter++;
    //    build(registry);
    //}
    void incrementalUpdate(ArenaRegistry& registry, uint8_t index);


    // Queries - these can run concurrently
    //void frustumCull(DualBVH::TraversalResult& result, const Frustum& f) const;

    void frustumCullWithOcclusion(DualBVH::TraversalResult& result, const Frustum& f,
                                             const glm::vec3& cameraPos,
                                             const glm::vec3& camWorldForward,
                                             const float& camForwardPosDot,
                                             ArenaRegistry* const registry,
                                             OcclusionMethod method, uint8_t index);

    TraversalResult broadPhaseCollision(uint8_t index) const;

    //std::vector<entt::entity> raycast(const Ray& r) const {
    //    std::vector<entt::entity> hits;
    //    if (isEmpty()) return hits;

    //    std::vector<uint32_t> nodeStack;
    //    nodeStack.reserve(64);
    //    nodeStack.push_back(rootIndex);

    //    while (!nodeStack.empty()) {
    //        uint32_t nodeIndex = nodeStack.back();
    //        nodeStack.pop_back();

    //        const BVHNode& node = nodes[currentFrame][nodeIndex];

    //        // Ray-AABB intersection test
    //        if (!node.bounds.intersects(r)) {
    //            continue;
    //        }

    //        if (node.isLeaf()) {
    //            for (uint32_t i = 0; i < node._pad0; ++i) {
    //                uint32_t primIndex = primitiveIndices[currentFrame][node.firstPrimitive + i];
    //                if (primitives[currentFrame][primIndex].bounds.getCoarseAABB().intersects(r)) {
    //                    hits.push_back(primitives[currentFrame][primIndex].entity);
    //                }
    //            }
    //        } else {
    //            if (node.leftChild != 0) nodeStack.push_back(node.leftChild);
    //            if (node.rightChild != 0) nodeStack.push_back(node.rightChild);
    //        }
    //    }

    //    return hits;
    //}
    //std::vector<entt::entity> raycast(const glm::vec3& origin,
    //                                  const glm::vec3& direction) const {
    //    return raycast(Ray{ origin, direction });
    //}


    // Management
    //void addPrimitive(entt::entity entity, const AABB& bounds) {
    //    if (entityToPrimitive.find(entity) != entityToPrimitive.end()) {
    //        return; // Already exists
    //    }

    //    uint32_t primIndex = primitives.size();
    //    primitives.emplace_back(entity, bounds);
    //    //primitives.back().worldBounds = bounds;  // Will be updated with transform
    //    primitives.back().frameUpdated = currentFrame;

    //    entityToPrimitive[entity] = primIndex;

    //    // Mark for rebuild (simple approach - could be optimized with insertion)
    //    dirtyCount = primitives.size();
    //}
    //void removePrimitive(entt::entity entity, uint8_t index) {
    //    auto it = entityToPrimitive[index].find(entity);
    //    if (it == entityToPrimitive[index].end()) return;

    //    uint32_t primIndex = it->second;

    //    // Swap with last element
    //    if (primIndex != primitives.size() - 1) {
    //        primitives[primIndex] = primitives.back();
    //        entityToPrimitive[index][primitives[index][primIndex].entity] = primIndex;
    //    }

    //    primitives[index].pop_back();
    //    entityToPrimitive[index].erase(it);

    //    // Mark for rebuild
    //    dirtyCount = primitives.size();
    //}
    //void markDirty(entt::entity entity, uint8_t index) {
    //    auto it = entityToPrimitive[index].find(entity);
    //    if (it != entityToPrimitive[index].end()) {
    //        uint32_t primIndex = it->second;
    //        if (primitives[index][primIndex].frameUpdated < currentFrame) {
    //            primitives[index][primIndex].frameUpdated = currentFrame - 1; // Force update
    //            dirtyCount++;
    //        }
    //    }
    //}
    //void nextFrame() {
    //    currentFrame++;
    //    dirtyCount = 0;
    //}

    // Thread-safe accessors
    std::vector<BVHNode>& getCurrentNodes() { return nodes[_threadIndexToUse.load()]; }
    uint32_t getNodeCount() const { return nodeCount[_threadIndexToUse.load()]; }
    uint32_t getPrimitiveCount(uint8_t index) const { return static_cast<uint32_t>(primitives[index].size()); }
    bool isEmpty(uint8_t index) const { return primitives[index].empty(); }

    // Debug/profiling methods
    //void printStats(uint8_t index) const {
    //    if (isEmpty()) {
    //        std::cout << "BVH is empty\n";
    //        return;
    //    }

    //    std::cout << "BVH Stats:\n";
    //    std::cout << "  Nodes: " << nodeCount << "\n";
    //    std::cout << "  Primitives: " << primitives[index].size() << "\n";
    //    std::cout << "  Rebuild count: " << rebuildCounter[index].load() << "\n";
    //    std::cout << "  Current frame: " << currentFrame[index].load() << "\n";

    //    // Calculate tree depth
    //    uint32_t maxDepth = 0;
    //    std::function<void(uint32_t, uint32_t)> calculateDepth =
    //        [&](uint32_t nodeIndex, uint32_t depth) {
    //        if (nodeIndex == 0) return;
    //        maxDepth = std::max(maxDepth, depth);

    //        const BVHNode& node = nodes[index][nodeIndex];
    //        if (!node.isLeaf()) {
    //            calculateDepth(node.leftChild, depth + 1);
    //            calculateDepth(node.rightChild, depth + 1);
    //        }
    //        };

    //    calculateDepth(rootIndex, 0);
    //    std::cout << "  Max depth: " << maxDepth << "\n";
    //}

    // Validate tree structure (debug)
    //bool validateTree(uint8_t index) const {
    //    if (isEmpty(index)) return true;


    //    std::function<bool(uint32_t, uint8_t)> validate = [&](uint32_t nodeIndex, uint8_t index) -> bool {
    //        if (nodeIndex == 0 || nodeIndex > nodeCount[index]) return false;

    //        const BVHNode& node = nodes[index][nodeIndex];

    //        if (node.isLeaf()) {
    //            // Validate leaf node
    //            if (node._pad0 == 0) return false;
    //            if (node.firstPrimitive + node._pad0 > primitiveIndices.size()) return false;

    //            // Check bounds contain all primitives
    //            for (uint32_t i = 0; i < node._pad0; ++i) {
    //                uint32_t primIndex = primitiveIndices[index][node.firstPrimitive + i];
    //                if (primIndex >= primitives.size()) return false;

    //                // Note: This is a simplified check - in practice you'd want 
    //                // more sophisticated bounds validation
    //            }
    //        } else {
    //            // Validate internal node
    //            if (node.leftChild == 0 && node.rightChild == 0) return false;

    //            if (node.leftChild != 0 && !validate(node.leftChild, index)) return false;
    //            if (node.rightChild != 0 && !validate(node.rightChild, index)) return false;
    //        }

    //        return true;
    //        };

    //    for (size_t i = 0; i < 2; ++i) {
    //        auto result = validate(rootIndex[i], index);
    //        if (!result)
    //            return false;
    //    }

    //    return true;
    //}

    struct Job { uint32_t nodeIndex; uint8_t planeMask; uint8_t state; };
};

// Usage example system
class BVHSystem
{
private:
    DualBVH m_bvh;
    uint32_t lastUpdateFrame = UINT32_INVALID;
    float counter = 4333424.0f;
    uint8_t currentIndex = UINT8_INVALID;

public:
    BVHSystem(ArenaRegistry& registry) :
        m_bvh{registry} {}
    // Called once per frame in main thread
    void updateBVH(ArenaRegistry& registry, double dt, double time) {
            //bvh.incrementalUpdate(registry);
            
        counter += static_cast<float>(dt);
        if (counter >= static_cast<float>(time)) {
            counter = 0;
            m_bvh.setPendingUpdate(currentIndex);
        }
        //bvh.nextFrame();
        //lastUpdateFrame = currentFrame;
        
    }

    DualBVH& bvh() { return m_bvh; }

    // Called from render thread
    //auto performFrustumCulling(DualBVH::TraversalResult& result, const Frustum& f, ArenaRegistry* const registry) const {
    //    return bvh.frustumCull(result, f);
    //}

    // Called from render thread with occlusion culling
    auto performFrustumCullingWithOcclusion(DualBVH::TraversalResult& result,
                                            const Camera* const cam,
                                            ArenaRegistry* const registry,
                                            DualBVH::OcclusionMethod method = DualBVH::OcclusionMethod::SIMPLE_DEPTH) { 
        if (!cam)
            return;

        auto zRow = cam->getZRow();
        return m_bvh.frustumCullWithOcclusion(
            result, cam->getFrustum(), cam->getPosition(), zRow.n, zRow.w, registry, method, currentIndex);
    }

    // Called from physics thread (can be concurrent with frustum culling)
    auto performBroadPhase() const {
        return m_bvh.broadPhaseCollision(0);
    }

    // Add/remove entities
    //void addEntity(entt::entity entity, const AABB& bounds) {
    //    bvh.addPrimitive(entity, bounds);
    //}

    //void removeEntity(entt::entity entity) {
    //    bvh.removePrimitive(entity);
    //}

    //void markEntityDirty(entt::entity entity) {
    //    bvh.markDirty(entity);
    //}

    //// Query methods
    //std::vector<entt::entity> raycast(const glm::vec3& origin, const glm::vec3& direction) const {
    //    return bvh.raycast(origin, direction);
    //}

    //// Debug/profiling
    ////void printStats() const { bvh.printStats(); }
    //bool validate() const { return bvh.validateTree(); }

    //// Force rebuild (useful for debugging or after major scene changes)
    //void forceRebuild(ArenaRegistry& registry) {
    //    bvh.rebuild(registry);
    //}
};

#endif