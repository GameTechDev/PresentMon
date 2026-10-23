# Minimal CMake for the PresentMon SDK C sample.
# Expects the installed SDK layout (header/lib/dll in the parent of this folder):
#
#   SDK/
#     PresentMonAPI.h
#     PresentMonAPI2Loader.lib
#     PresentMonAPI2Loader.dll
#     Samples/
#       main.c
#       CMakeLists.txt
#
# Configure from this directory, optionally overriding the SDK root:
#   cmake -S . -B build -DPRESENTMON_SDK_DIR="C:/Program Files/Intel/PresentMon/SDK"
#   cmake --build build --config Release

cmake_minimum_required(VERSION 3.16)
project(PresentMonSdkSample C)

if(NOT PRESENTMON_SDK_DIR)
    get_filename_component(PRESENTMON_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
endif()

set(PMON_API_HEADER "${PRESENTMON_SDK_DIR}/PresentMonAPI.h")
set(PMON_LOADER_LIB "${PRESENTMON_SDK_DIR}/PresentMonAPI2Loader.lib")
set(PMON_LOADER_DLL "${PRESENTMON_SDK_DIR}/PresentMonAPI2Loader.dll")

if(NOT EXISTS "${PMON_API_HEADER}")
    message(FATAL_ERROR "PresentMonAPI.h not found at ${PMON_API_HEADER}. Set PRESENTMON_SDK_DIR.")
endif()
if(NOT EXISTS "${PMON_LOADER_LIB}")
    message(FATAL_ERROR "PresentMonAPI2Loader.lib not found at ${PMON_LOADER_LIB}. Set PRESENTMON_SDK_DIR.")
endif()

add_executable(fps_gpu_sample main.c)
target_include_directories(fps_gpu_sample PRIVATE "${PRESENTMON_SDK_DIR}")
target_link_libraries(fps_gpu_sample PRIVATE "${PMON_LOADER_LIB}")

if(EXISTS "${PMON_LOADER_DLL}")
    add_custom_command(TARGET fps_gpu_sample POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${PMON_LOADER_DLL}"
            "$<TARGET_FILE_DIR:fps_gpu_sample>/PresentMonAPI2Loader.dll"
        COMMENT "Copy PresentMonAPI2Loader.dll next to fps_gpu_sample"
    )
endif()
