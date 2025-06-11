# CMakeList.txt : CMake project for MobaBase, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("MobaBase")

# Add source to this project's executable.
add_executable (MobaBase "main.cpp"
	"Public/Debug.hpp"
	"Public/Engine.h"
	"Public/InputManager.hpp"
	"Public/Debug.hpp"
	"Public/WindowSurface.h"

	"Private/Engine.cpp"
	"Private/WindowSurface.cpp")


target_include_directories(MobaBase PRIVATE "Public/")

# DEFINES
target_compile_definitions(MobaBase PRIVATE BUILD_WIN)

# NVRHI
add_subdirectory(External/nvrhi)
target_link_libraries(MobaBase PRIVATE nvrhi)

# GLM
add_subdirectory(External/glm)
target_link_libraries(MobaBase PRIVATE glm::glm-header-only)