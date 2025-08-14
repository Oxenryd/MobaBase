cmake_minimum_required (VERSION 3.21)

project ("MobaBase")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Only apply to MSVC and Release builds
if(MSVC)
    message(STATUS "Enabling /GL and /LTCG for MSVC Release build")

    # Enable /GL for compile step
    add_compile_options(/GL)

    # Enable /LTCG for link step
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")

endif()

# FETCHES -----------------------------------------------------------------------------
include(FetchContent)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

#EnTT
FetchContent_Declare(
  EnTT
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG        v3.13.1  # or latest stable tag; check the GitHub repo
)
FetchContent_MakeAvailable(EnTT)

#Boost
FetchContent_Declare(
  boost_sort
  GIT_REPOSITORY https://github.com/boostorg/sort.git
  GIT_TAG        boost-1.85.0
)

#SPIR-V reflect
FetchContent_Declare(
  spirv-reflect
  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
  GIT_TAG        main  # or use a specific commit or tag
)
set(SPIRV_REFLECT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_TOOLS OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spirv-reflect)

# nlohmann/json
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3 # Use latest stable or pin to your desired version
)
FetchContent_MakeAvailable(json)

# AssImp
include(FetchContent)
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG v6.0.2
)
# set(ASSIMP_BUILD_EXPORT OFF CACHE BOOL "Disable Assimp export support")

# FORCE STATIC BUILD
set(ASSIMP_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) # global fallback

FetchContent_MakeAvailable(assimp)

get_property(all_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
message(STATUS "Available targets: ${all_targets}")


# SOURCES -----------------------------------------------------------------------------
add_executable (MobaBase
	"main.cpp"

	"Public/Debug.hpp"
	"Public/Engine.h"
	"Public/InputManager.hpp"
	"Public/Debug.hpp"
	"Public/WindowSurface.h"
	"Public/Event.hpp"
	"Public/Log.hpp"
	"Public/Delegate.hpp"
	"Public/VulkanContext.hpp"
	"Public/Bits.hpp"
	"Public/TimerSystem.h"
	"Public/Timer.h"
	"Public/Arena.hpp"
	"Public/FSM.hpp"	
	"Public/Material.hpp"
	"Public/Shader.h"
	"Public/ArenaAllocator.hpp"
	"Public/MemoryProviderConcept.h"
	"Public/RenderManager.h"
	"Public/ErrorCodes.hpp"
	"Public/FileSys.h"
	"Public/HlslTypes.h"
	"Public/FrameData.hpp"
	"Public/Scene.h"
	"Public/Templates.hpp"
	"Public/IRenderProvider.h"
	"Public/Texture.hpp"
	"Public/Hashes.hpp"
	"Public/MaterialParams.h"
	"Public/TypeReflection.hpp"
	"Public/GlobalMacros.h"
	"Public/MaterialStates.h"
	"Public/Transform.hpp"
	"Public/SystemECS.h"
	"Public/TransformSystem.hpp"
	"Public/ObjectState.hpp"
	"Public/MobaMath.hpp"
	"Public/EnabledTag.hpp"
	"Public/GameObjectSystem.hpp"
	"Public/GameObject.hpp"
	"Public/Tag.hpp"
	"Public/TagSystem.hpp"
	"Public/EnabledTag.hpp"
	"Public/SceneRenderSystem.hpp"
	"Public/FixedString.hpp"
	"Public/DrawCommand.hpp"
	"Public/MeshComponent.hpp"
	"Public/AssetLoader.h"
	"Public/InputTypes.h"
	"Public/Camera.hpp"
	"Public/RenderTarget.hpp"
	"Public/GlobalComponents.hpp"
	"Public/GlobalSystem.hpp"
	"Public/Timing.h"
	"Public/MemDebugFilter.hpp"
	"Public/BasicTypes.hpp"
	"Public/stb_image.h"
	"Public/stb_image_write.h"
	"Public/BoundingSystem.h"
	"Public/LightSystem.hpp"
	"Public/Light.hpp"
	"Public/BVH.hpp"
	"Public/BoundingVolume.hpp"
	"Public/Frustum.hpp"
	"Public/TransformComponent.hpp"


	"Private/Engine.cpp"
	"Private/Timer.cpp"
	"Private/TimerSystem.cpp"
	"Private/RenderManager.cpp"
	"Private/FileSys.cpp"
	"Private/Shader.cpp"
    "Private/Material.cpp"
	"Private/MaterialParams.cpp"
	"Private/SystemECS.cpp" 
	"Private/GameObjectSystem.cpp"
	"Private/Transform.cpp"
	"Private/AssetLoader.cpp"
	"Private/VulkanContext.cpp"
	"Private/SceneRenderSystem.cpp"
	"Private/GlobalComponents.cpp"
	"Private/Camera.cpp"
	"Private/Timing.cpp"
	"Private/Texture.cpp"
	"Private/LightSystem.cpp"
	"Private/Mesh.cpp"
	"Private/Light.cpp"
	"Private/BoundingVolume.cpp"
	"Private/BVH.cpp")



# LINK -------------------------------------------------------------------------------
find_package(Vulkan REQUIRED)

target_link_libraries(MobaBase PRIVATE
    glm::glm
	Vulkan::Vulkan
	EnTT::EnTT
	spirv-reflect-static
	nlohmann_json
	assimp::assimp
)

target_include_directories(MobaBase PRIVATE 
    "Public/"
	${boost_sort_SOURCE_DIR}/include
)



# DEFINES ----------------------------------------------------------------------------
target_compile_definitions(MobaBase PRIVATE 
	GLM_FORCE_SSE2
	DEBUGGING
	BUILD_WIN
	# $<$<CONFIG:Debug>:LOGGING>
	LOGGING
	IGPU_PRIO	
	SHADER_HOTRELOAD

	# VULKAN SPECIFICS
	VULKAN_GLOBAL_DESCRIPTOR_SETS=3
	# $<$<CONFIG:Debug>:VULKAN_VALIDATION>
	VULKAN_VALIDATION
	VULKAN_FRAMES_IN_FLIGHT=2
	VULKAN_SWAPCHAINS=3
	VULKAN_MAX_RUNTIMEARRAY_INSTANCES=1048576
	VULKAN_MAX_TEXTURE_SSBO=512

	VULKAN_INSTANCE_INDEX_STAGE_SIZE=16777216

	VULKAN_DESCPOOL_BASE_UNIFORMS=2560
	VULKAN_DESCPOOL_BASE_STORAGE=2560
	VULKAN_DESCPOOL_BASE_SAMPLER=64
	VULKAN_DESCPOOL_BASE_IMAGE=2560

	VULKAN_DESCPOOL_DYNAMIC_UNIFORMS=1024
	VULKAN_DESCPOOL_DYNAMIC_STORAGE=1024
	VULKAN_DESCPOOL_DYNAMIC_SAMPLER=128
	VULKAN_DESCPOOL_DYNAMIC_IMAGE=128

	VULKAN_LIGHT_CLUSTERS_X=64
	VULKAN_LIGHT_CLUSTERS_Y=36
	VULKAN_LIGHT_CLUSTERS_Z=24
	VULKAN_LIGHT_CLUSTER_THREADS_X=4
	VULKAN_LIGHT_CLUSTER_THREADS_Y=4
	VULKAN_LIGHT_CLUSTER_THREADS_Z=4

	# Built in Shaders
	SHADER_BASE_LIGHTCLUSTER_CS=\"LightClusterCS\"
	SHADER_BASE_VS=\"BaseVS\"
	SHADER_BASE_PS=\"BasePS\"
	SHADER_SHAPERENDERER_VS=\"ShapeRendererVS\"
	SHADER_SHAPERENDERER_PS=\"ShapeRendererPS\"


	# Built In Mat Properties
	MAT_CAMERADATA_NAME=\"cameraData\"
	MAT_CAMERADATA_SET=0
	MAT_CAMERADATA_BIND=0

	MAT_SAMPLER_NAME=\"smp\"
	MAT_SAMPLER_SET=1
	MAT_SAMPLER_BIND=1

	MAT_MODELMATRICES_NAME=\"modelMatrices\"
	MAT_MODELMATRICES_SET=0
	MAT_MODELMATRICES_BIND=2

	MAT_TEXTURES_NAME=\"textures\"
	MAT_TEXTURES_SET=1
	MAT_TEXTURES_BIND=3

	MAT_BASE_MAT_INSTANCES_INDICES_NAME=\"instanceData\"
	MAT_BASE_MAT_INSTANCES_INDICES_SET=0
	MAT_BASE_MAT_INSTANCES_INDICES_BIND=3

	MAT_BASE_MAT_INSTANCES_NAME=\"baseMatInstances\"
	MAT_BASE_MAT_INSTANCES_SET=1
	MAT_BASE_MAT_INSTANCES_BIND=2

	MAT_GPULIGHTS_NAME=\"lights\"
	MAT_GPULIGHTS_SET=2
	MAT_GPULIGHTS_BIND=0

	MAT_CLUSTER_LIGHTCOUNT_NAME=\"clusterLightCount\"
	MAT_CLUSTER_LIGHTCOUNT_SET=2
	MAT_CLUSTER_LIGHTCOUNT_BIND=1

	MAT_CLUSTER_LIGHTINDICES_NAME=\"clusterLightIndices\"
	MAT_CLUSTER_LIGHTINDICES_SET=2
	MAT_CLUSTER_LIGHTINDICES_BIND=2

	# Data types settings
	FIXED_STRINGS_MAX_LENGTH=63

	# ECS
	ECS_BASE_COMPONENTS_RESERVATION_COUNT=2048
	
	# System
	LAYERS_FIELD_TYPE=uint32_t
	SCENE_MAX_SCENES=512
	RENDER_SCENE_MAX_THREADS=128

	# Directories
	SOURCE_DIR=\"${CMAKE_SOURCE_DIR}/\"
	ASSETS_DIR=\"${CMAKE_SOURCE_DIR}/Assets/\"
	SHADER_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}/Shaders\"
	SHADER_TARGET_DIR=\"${CMAKE_BINARY_DIR}/Shaders/Build\"
	DXC_DIR=\"${CMAKE_SOURCE_DIR}/Ext/dxc\"
	SHADER_TYPE_VERTEX=\"ShaderType:Vertex\"
	SHADER_TYPE_FRAGMENT=\"ShaderType:Fragment\"
	SHADER_TYPE_COMPUTE=\"ShaderType:Compute\"
	SHADER_NAME=\"Name:\"
)


# FILE ACTIONS -----------------------------------------------------------------------
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Shaders/Build")