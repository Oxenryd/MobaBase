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
	"Public/Shader.hpp"
	"Public/ArenaAllocator.hpp"
	"Public/MemoryProviderConcept.h"
	"Public/MathTypes.h"
	"Public/BaseTypesHLSL.h"
	"Public/ShaderManager.h"
	"Public/ErrorCodes.hpp"
	"Public/FS.h"

	"Private/Engine.cpp"
	"Private/Timer.cpp"
	"Private/TimerSystem.cpp"
	"Private/ShaderManager.cpp"
	"Private/FS.cpp"
)



# LINK -------------------------------------------------------------------------------
find_package(Vulkan REQUIRED)

target_link_libraries(MobaBase PRIVATE
    glm::glm
	Vulkan::Vulkan
)

target_include_directories(MobaBase PRIVATE 
    "Public/"
)



# DEFINES ----------------------------------------------------------------------------
target_compile_definitions(MobaBase PRIVATE 
	DEBUGGING
	BUILD_WIN
	LOGGING
	USE_VULKAN
	#VULKAN_VALIDATION
	VULKAN_MAX_FRAMES_IN_FLIGHT=3
	SHADER_HOTRELOAD

	# Directories
	SHADER_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}/Shaders\"
	SHADER_TARGET_DIR=\"${CMAKE_BINARY_DIR}/Shaders/Build\"
	DXC_DIR=\"${CMAKE_SOURCE_DIR}/Ext/dxc\"
	SHADER_TYPE_VERTEX=\"ShaderType:Vertex\"
	SHADER_TYPE_FRAGMENT=\"ShaderType:Pixel\"
	SHADER_TYPE_COMPUTE=\"ShaderType:Compute\"
)


# FILE ACTIONS -----------------------------------------------------------------------

#add_custom_target(CleanShaderBuildFolder
#    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/Shaders"
#    COMMENT "Cleaning shader build output folder"
#)
#add_dependencies(MobaBase CleanShaderBuildFolder)

# Copy Shaders
#add_custom_command(
#    TARGET MobaBase POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory
#            "${CMAKE_SOURCE_DIR}/Shaders"
#            "$<TARGET_FILE_DIR:MobaBase>/Shaders"
#    COMMENT "Copying Shaders directory to output"
#)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Shaders/Build")

#add_custom_target(CopyShaders ALL
#    COMMAND ${CMAKE_COMMAND} -E copy_directory
#            "${CMAKE_SOURCE_DIR}/Shaders"
#            "${CMAKE_BINARY_DIR}/Shaders"
#    COMMENT "Copying Shaders directory"
#)
#add_dependencies(MobaBase CopyShaders)


# Copy DXC
#add_custom_command(
#    TARGET MobaBase POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory
#            "${CMAKE_SOURCE_DIR}/Ext/dxc"
#            "$<TARGET_FILE_DIR:MobaBase>/Shaders/dxc"
#    COMMENT "Copying DXC directory to output"
#)