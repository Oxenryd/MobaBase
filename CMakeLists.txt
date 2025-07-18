cmake_minimum_required (VERSION 3.21)

project ("MobaBase")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
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
	"Public/GraphicContext.h"
	"Public/Material.hpp"
	"Public/Shader.h"
	"Public/ArenaAllocator.hpp"
	"Public/MemoryProviderConcept.h"
	"Public/MathTypes.h"
	"Public/BaseTypesHLSL.h"
	"Public/ShaderManager.h"
	"Public/ErrorCodes.hpp"
	"Public/FS.h"
	"Public/HlslTypes.h"
	"Public/Transform.h"
	"Public/FrameData.hpp"
	"Public/Scene.h"
	"Public/Templates.hpp"
	"Public/PSO.hpp"
	"Public/IShaderProvider.h"
	"Public/Texture.hpp"
	"Public/Hashes.hpp"
	"Public/MaterialParams.h"
	"Public/TypeReflection.hpp"
	"Public/GlobalMacros.h"
	"Public/MaterialStates.h"
	
	"Private/Engine.cpp"
	"Private/Timer.cpp"
	"Private/TimerSystem.cpp"
	"Private/ShaderManager.cpp"
	"Private/FS.cpp"
    "Private/Scene.cpp"
	"Private/Shader.cpp"
    "Private/Material.cpp"
	"Private/MaterialParams.cpp"
  )



# LINK -------------------------------------------------------------------------------
find_package(Vulkan REQUIRED)

target_link_libraries(MobaBase PRIVATE
    glm::glm
	Vulkan::Vulkan
	EnTT::EnTT
	spirv-reflect-static
	nlohmann_json
)

target_include_directories(MobaBase PRIVATE 
    "Public/"
	${boost_sort_SOURCE_DIR}/include
)



# DEFINES ----------------------------------------------------------------------------
target_compile_definitions(MobaBase PRIVATE 
	DEBUGGING
	BUILD_WIN
	LOGGING
	VULKAN_GLOBAL_DESCRIPTOR_SETS=2
	VULKAN_VALIDATION
	#IGPU_PRIO
	VULKAN_MAX_FRAMES_IN_FLIGHT=3
	SHADER_HOTRELOAD
	MAX_RUNTIMEARRAY_INSTANCES=1048576

	# Directories
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