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

	"Private/Engine.cpp"
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
	BUILD_WIN
	LOGGING
	USE_VULKAN
)
