
# ge - ReXGlue Recompiled Project
#
# This file is yours to edit. 'rexglue migrate' will NOT overwrite it.
# SDK integration lives in cmake/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(ge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/rexglue.cmake)

# Sources
set(GE_SOURCES
    src/main.cpp
    src/ge_hooks.cpp
    src/ge_gpu_hooks.cpp
    src/ge_menu.cpp
    src/ge_postfx.cpp
    src/ge_username.cpp
    src/ge_ce_patches.cpp
)

if(WIN32)
    # Embed the application icon (GoldenEye.rc -> "MAINICON"). Use LLVM's resource
    # compiler since the toolchain is clang/lld (MSVC rc.exe may not be on PATH).
    if(NOT CMAKE_RC_COMPILER)
        set(CMAKE_RC_COMPILER "llvm-rc")
    endif()
    enable_language(RC)
    add_executable(ge WIN32 ${GE_SOURCES} GoldenEye.rc)
else()
    add_executable(ge ${GE_SOURCES})
endif()

# Output a GoldenEye.exe (the in-Explorer/process name), independent of the
# internal target/app name "ge" (which still drives ge.toml / the user data dir).
set_target_properties(ge PROPERTIES OUTPUT_NAME "GoldenEye")

rexglue_setup_target(ge GPU_PLUGINS xenos)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(ge_recomp PRIVATE
        -include "${CMAKE_CURRENT_SOURCE_DIR}/src/ge_recomp_compat.h")
endif()

# The SDK sets an $ORIGIN RPATH on Linux, so stage its shared runtime there.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_custom_command(TARGET ge POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:rex::runtime> $<TARGET_FILE_DIR:ge>
        VERBATIM)
    if(TARGET rex::TracyClient)
        add_custom_command(TARGET ge POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:rex::TracyClient> $<TARGET_FILE_DIR:ge>
            VERBATIM)
    endif()
endif()
