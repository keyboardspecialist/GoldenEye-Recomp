# Source-controlled SDK integration: a fresh checkout has no generated files.
# Accept the SDK unpacked next to the project, or a normal installed package.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        set(_ge_sdk_platform linux-amd64)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
        set(_ge_sdk_platform linux-arm64)
    endif()
    if(_ge_sdk_platform)
        list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/rexglue/${_ge_sdk_platform}")
    endif()
endif()
find_package(Threads REQUIRED)

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/generated/rexglue.cmake" OR
   NOT EXISTS "${PROJECT_SOURCE_DIR}/generated/sources.cmake")
    message(FATAL_ERROR
        "Generated game code is missing. Put your game files in assets/ (including "
        "assets/default.xex), then run: rexglue codegen ge_manifest.toml")
endif()
include("${PROJECT_SOURCE_DIR}/generated/rexglue.cmake")
