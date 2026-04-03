cmake_minimum_required (VERSION 3.51)

project ("MobaBase")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Only apply to MSVC and Release builds
if(MSVC)
	message(STATUS "Enabling /GL and /LTCG for MSVC Release build")

	# Enable /GL for compile step
	add_compile_options("$<$<CONFIG:Release>:/GL>")
	add_link_options("$<$<CONFIG:Release>:/LTCG>")
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
    "$<$<CONFIG:Debug,Release,RelWithDebInfo>:ProgramDatabase>")
	# Enable /LTCG for link step
	#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
	#set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
	#set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
else()
	add_compile_options(-march=native)
	add_compile_options(-Wno-template-body)
endif()

# FETCHES -----------------------------------------------------------------------------
include(FetchContent)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
)

set(GLM_FORCE_INTRINSICS        ON CACHE BOOL "" FORCE)
#set(GLM_FORCE_ALIGNED_GENTYPES  ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glm)


#EnTT
FetchContent_Declare(
  EnTT
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG        v3.16.0  # or latest stable tag; check the GitHub repo
)
FetchContent_MakeAvailable(EnTT)

#Boost ------------------

# Sort
FetchContent_Declare(
  boost_sort
  GIT_REPOSITORY https://github.com/boostorg/sort.git
  GIT_TAG        boost-1.85.0
)
FetchContent_MakeAvailable(boost_sort)

FetchContent_Declare(
		boost_unordered
		GIT_REPOSITORY https://github.com/MikePopoloski/boost_unordered.git
		GIT_TAG        v1.85.0
)
FetchContent_MakeAvailable(boost_unordered)

# ----------------------------------------------------------------



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
if (TARGET assimp)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		# Disable the specific boundary checks that GCC 15 is tripping on
		target_compile_options(assimp PRIVATE -Wno-array-bounds -Wno-error=array-bounds)

		# Newer GCCs also hate some of Assimp's string handling
		target_compile_options(assimp PRIVATE -Wno-stringop-overflow -Wno-error=stringop-overflow)
	endif()
endif()





# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)



# IMGUI
include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking                # pin to a commit for stability if you prefer
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_PLATFORM_BACKEND imgui_impl_glfw.cpp)
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
  ${imgui_SOURCE_DIR}/backends/${IMGUI_PLATFORM_BACKEND}
)
target_include_directories(imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

get_property(all_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
message(STATUS "Available targets: ${all_targets}")


# SOURCES -----------------------------------------------------------------------------
#add_executable (MobaBase
#		Public/API.h
#		Public/Format_fixes.hpp)

add_executable(MobaBase)

target_sources(MobaBase
	PRIVATE
		"main.cpp"
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
		"Private/Mesh.cpp"
		"Private/Light.cpp"
		"Private/BoundingVolume.cpp"
		"Private/BoundingSystem.cpp"
		"Private/BVH.cpp"
		"Private/Scene.cpp"
)




# LINK -------------------------------------------------------------------------------
find_package(Vulkan REQUIRED)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan)
target_link_libraries(imgui PUBLIC glfw)

target_link_libraries(MobaBase PRIVATE
    glm::glm
	Vulkan::Vulkan
	EnTT::EnTT
	spirv-reflect-static
	nlohmann_json
	assimp::assimp
	glfw
)

target_include_directories(MobaBase PRIVATE 
    "Public/"
	${boost_unordered_SOURCE_DIR}/include
	${boost_sort_SOURCE_DIR}/include
)



target_compile_options(MobaBase PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -mavx2 -mfma> #-Wpedantic
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /wd4201 /wd4127 /wd4100 /wd4458 /wd4505 /wd4324 /wd6262>
)

# DEFINES ----------------------------------------------------------------------------
target_compile_definitions(MobaBase PRIVATE
	GLFW_INCLUDE_VULKAN
	#BUILD_WIN
	BUILD_DLL
	BUILD_GLFW
	# X11
	WAYLAND
	GLM_FORCE_SSE2
	GLM_ENABLE_EXPERIMENTAL
	DEBUGGING
	# $<$<CONFIG:Debug>:LOGGING>
	LOGGING
	PROFILER
	#IGPU_PRIO	
	#SHADER_HOTRELOAD

	# VULKAN SPECIFICS
	VULKAN_GLOBAL_DESCRIPTOR_SETS=3
	# $<$<CONFIG:Debug>:VULKAN_VALIDATION>
	VULKAN_VALIDATION
	VULKAN_FRAMES_IN_FLIGHT=2
	VULKAN_SWAPCHAINS=3
	VULKAN_MAX_RUNTIMEARRAY_INSTANCES=1048576
	VULKAN_MAX_TEXTURE_SSBO=512
	VULKAN_MATSTAGEBUF_SIZE=128_MB
	VULKAN_STAGE_BUFFERS=2

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

	MAT_BASE_INSTANCES_DATA_NAME=\"instanceData\"
	MAT_BASE_INSTANCES_DATA_SET=0
	MAT_BASE_INSTANCES_DATA_BIND=3

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
	FIXED_STRINGS_MAX_LENGTH=62

	# ECS
	ECS_BASE_COMPONENTS_RESERVATION_COUNT=2048
	
	# System
	ENGINE_NAME=\"MobaEng\"
	LAYERS_FIELD_TYPE=uint32_t
	SCENE_MAX_SCENES=512
	RENDER_SCENE_MAX_THREADS=128
	TRANSFORM_THREADS_MAX=16
	WORK_QUEUE_CAP=1024
	PROFILE_PATH=\"${CMAKE_SOURCE_DIR}/trace.json\"
	INPUT_RESO_SCALE_FACTOR=0.0001f

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

# DX Compiler
set(DXC_SOURCE_PATH "${CMAKE_SOURCE_DIR}/Ext/dxc")
if(WIN32)
	set(DXC_FILES
			"${DXC_SOURCE_PATH}/dxc.exe"
			"${DXC_SOURCE_PATH}/dxcompiler.dll"
			"${DXC_SOURCE_PATH}/dxil.dll"
	)
else()
	set(DXC_FILES
			"${DXC_SOURCE_PATH}/dxc"
			"${DXC_SOURCE_PATH}/libdxcompiler.so"
			"${DXC_SOURCE_PATH}/libdxil.so"
	)
endif()
foreach(dxc_file ${DXC_FILES})
	file(COPY ${dxc_file} DESTINATION ${CMAKE_BINARY_DIR})
endforeach()
if(NOT WIN32)
	# Ensure the 'dxc' binary is actually executable after copy
	add_custom_command(TARGET MobaBase POST_BUILD
			COMMAND chmod +x $<TARGET_FILE_DIR:MobaBase>/dxc
			COMMENT "Setting executable permissions on DXC"
	)
endif()