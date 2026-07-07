# Commands.cmake
# Reusable helper functions for the build.

# get_sources_from_path(<out_var> <path> [RECURSE] [EXTENSIONS ext1 ext2 ...])
#
# Collects all source/header files under <path> and stores the list in <out_var>
# in the caller's scope.
#
# Options:
#   RECURSE              - search subdirectories too (default: off)
#   EXTENSIONS ext1 ext2  - override the default extension list (without dots, e.g. c cpp h)
#
# Example:
#   get_sources_from_path(MY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src")
#   get_sources_from_path(MY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src" RECURSE)
#   get_sources_from_path(MY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src" EXTENSIONS c h)
#
function(get_sources_from_path OUT_VAR PATH_ARG)
    set(options RECURSE)
    set(multiValueArgs EXTENSIONS)
    cmake_parse_arguments(ARG "${options}" "" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_EXTENSIONS)
        set(ARG_EXTENSIONS c cpp cc cxx h hpp hh hxx)
    endif()

    set(GLOB_PATTERNS "")
    foreach(ext ${ARG_EXTENSIONS})
        list(APPEND GLOB_PATTERNS "${PATH_ARG}/*.${ext}")
    endforeach()

    if(ARG_RECURSE)
        file(GLOB_RECURSE FOUND_SOURCES CONFIGURE_DEPENDS ${GLOB_PATTERNS})
    else()
        file(GLOB FOUND_SOURCES CONFIGURE_DEPENDS ${GLOB_PATTERNS})
    endif()

    if(NOT FOUND_SOURCES)
        message(WARNING "get_sources_from_path: no sources found in '${PATH_ARG}'")
    endif()

    # Push the result back to the caller's scope
    set(${OUT_VAR} ${FOUND_SOURCES} PARENT_SCOPE)
endfunction()

# add_include_from_path(<target> <path> [PUBLIC|PRIVATE|INTERFACE])
function(add_include_from_path TARGET_NAME PATH_ARG)
    set(oneValueArgs SCOPE)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

    if(NOT ARG_SCOPE)
        set(ARG_SCOPE PRIVATE)
    endif()

    target_include_directories(${TARGET_NAME} ${ARG_SCOPE} "${PATH_ARG}")
endfunction()

# configure_clangd(<clangd_file_path>)
#
# Writes/updates a .clangd file so its CompileFlags.CompilationDatabase
# always points at the current build directory. Safe to call every
# configure - it only touches the CompilationDatabase line, preserving
# anything else you've added to the file manually.
#
# Example:
#   configure_clangd("${CMAKE_SOURCE_DIR}/.clangd")
#
function(configure_clangd CLANGD_PATH)
    set(DB_PATH "${CMAKE_BINARY_DIR}")

    if(EXISTS "${CLANGD_PATH}")
        file(READ "${CLANGD_PATH}" CLANGD_CONTENTS)

        if(CLANGD_CONTENTS MATCHES "CompilationDatabase:[ \t]*[^\n]*")
            # Replace existing CompilationDatabase line
            string(REGEX REPLACE
                "CompilationDatabase:[ \t]*[^\n]*"
                "CompilationDatabase: ${DB_PATH}"
                CLANGD_CONTENTS "${CLANGD_CONTENTS}"
            )
            file(WRITE "${CLANGD_PATH}" "${CLANGD_CONTENTS}")
            message(STATUS "configure_clangd: updated CompilationDatabase -> ${DB_PATH}")
        elseif(CLANGD_CONTENTS MATCHES "CompileFlags:")
            # CompileFlags block exists but no CompilationDatabase key - insert one
            string(REGEX REPLACE
                "(CompileFlags:[ \t]*\n)"
                "\\1  CompilationDatabase: ${DB_PATH}\n"
                CLANGD_CONTENTS "${CLANGD_CONTENTS}"
            )
            file(WRITE "${CLANGD_PATH}" "${CLANGD_CONTENTS}")
            message(STATUS "configure_clangd: inserted CompilationDatabase -> ${DB_PATH}")
        else()
            # File exists but has neither key - append a block
            file(APPEND "${CLANGD_PATH}" "\nCompileFlags:\n  CompilationDatabase: ${DB_PATH}\n")
            message(STATUS "configure_clangd: appended CompileFlags block -> ${DB_PATH}")
        endif()
    else()
        # No .clangd yet - create a minimal one
        file(WRITE "${CLANGD_PATH}" "CompileFlags:\n  CompilationDatabase: ${DB_PATH}\n")
        message(STATUS "configure_clangd: created ${CLANGD_PATH} -> ${DB_PATH}")
    endif()
endfunction()