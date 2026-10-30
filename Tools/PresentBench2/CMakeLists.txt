include(PresentBench2Sdks)
pmon_configure_presentbench2_sdks()

find_package(nlohmann_json CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

set(PRESENTBENCH2_CORE_SOURCES
    CliOptions.cpp
    Window.cpp
    BackendFactory.cpp
    Capabilities.cpp
    Dx12PresentComponent.cpp
    ControlServer.cpp
    App.cpp
    FrameLoop.cpp
    LogSetup.cpp
    CubeScene.cpp
    backends/DxgiCommon.cpp
    backends/HlslCompile.cpp
    backends/Dx9Backend.cpp
    backends/Dx11Backend.cpp
    backends/Dx12Backend.cpp
    backends/VulkanBackend.cpp
    markers/ProviderSink.cpp
    markers/PclStatsSink.cpp
    "${PROJECT_SOURCE_DIR}/Provider/PresentMonProvider.cpp"
)

add_library(PresentBench2Core STATIC ${PRESENTBENCH2_CORE_SOURCES})
target_compile_definitions(
    PresentBench2Core
    PUBLIC
        _CONSOLE
        NOMINMAX
    PRIVATE
        $<$<BOOL:${PMON_PRESENTBENCH2_HAS_XESS_FG}>:PMON_PRESENTBENCH2_HAS_XESS_FG>
)
target_include_directories(
    PresentBench2Core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Provider"
        "${PROJECT_SOURCE_DIR}/IntelPresentMon"
)
target_link_libraries(
    PresentBench2Core
    PUBLIC
        pmon::build_options
        pmon::warnings_default
        pmon::release_link_policy
        pmon::common_utilities
        nlohmann_json::nlohmann_json
        Vulkan::Vulkan
        d3d9
        d3d11
        d3d12
        d3dcompiler
        dxgi
        dxguid
        shell32
        user32
        gdi32
        advapi32
)

if(PMON_PRESENTBENCH2_HAS_XESS_FG)
    target_sources(PresentBench2Core PRIVATE vendor/XessFgAdapter.cpp)
    target_include_directories(
        PresentBench2Core
        PRIVATE
            "${PMON_PRESENTBENCH2_XESS_FG_ROOT}"
            "${PMON_PRESENTBENCH2_XESS_FG_ROOT}/inc"
    )
    target_link_libraries(
        PresentBench2Core
        PRIVATE
            "${PMON_PRESENTBENCH2_XESS_FG_ROOT}/lib/libxess_fg.lib"
            "${PMON_PRESENTBENCH2_XESS_FG_ROOT}/lib/libxell.lib"
    )
endif()

add_executable(PresentBench2 main.cpp)
target_link_libraries(PresentBench2 PRIVATE PresentBench2Core)

if(PMON_PRESENTBENCH2_HAS_LOCAL_VULKAN_VALIDATION_LAYERS)
    set(
        _pb2_validation_layer_root
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}"
    )
    set(
        _pb2_validation_layer_release_dir
        "${_pb2_validation_layer_root}/bin"
    )
    set(
        _pb2_validation_layer_debug_dir
        "${_pb2_validation_layer_root}/debug/bin"
    )
    foreach(_pb2_layer_config IN ITEMS release debug)
        set(_pb2_layer_dir "${_pb2_validation_layer_${_pb2_layer_config}_dir}")
        foreach(
            _pb2_layer_file
            IN ITEMS
                VkLayer_khronos_validation.dll
                VkLayer_khronos_validation.json
        )
            if(NOT EXISTS "${_pb2_layer_dir}/${_pb2_layer_file}")
                message(
                    FATAL_ERROR
                    "PresentBench2 Vulkan validation layer file is missing: "
                    "${_pb2_layer_dir}/${_pb2_layer_file}"
                )
            endif()
        endforeach()
    endforeach()
    set(
        _pb2_validation_layer_source_dir
        "$<IF:$<CONFIG:Debug>,${_pb2_validation_layer_debug_dir},${_pb2_validation_layer_release_dir}>"
    )
    set(
        _pb2_validation_layer_output_dir
        "$<TARGET_FILE_DIR:PresentBench2>/vulkan-layers"
    )
    add_custom_command(
        TARGET PresentBench2
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E make_directory
            "${_pb2_validation_layer_output_dir}"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${_pb2_validation_layer_source_dir}/VkLayer_khronos_validation.dll"
            "${_pb2_validation_layer_source_dir}/VkLayer_khronos_validation.json"
            "${_pb2_validation_layer_output_dir}"
        VERBATIM
    )
endif()

if(PMON_PRESENTBENCH2_HAS_XESS_FG)
    foreach(_pb2_dll IN LISTS PMON_PRESENTBENCH2_XESS_FG_RUNTIME_DLLS)
        add_custom_command(
            TARGET PresentBench2
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_pb2_dll}"
                "$<TARGET_FILE_DIR:PresentBench2>"
            VERBATIM
        )
    endforeach()
endif()

set_target_properties(
    PresentBench2
    PROPERTIES
        FOLDER "Tools"
        OUTPUT_NAME "PresentBench2"
)
set_target_properties(PresentBench2Core PROPERTIES FOLDER "Tools")

if(PMON_BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
    add_executable(
        PresentBench2Tests
        tests/CubeSceneTests.cpp
        tests/FrameLoopTests.cpp
        tests/ControlServerTests.cpp
        tests/CliRejectTests.cpp
    )
    target_link_libraries(
        PresentBench2Tests
        PRIVATE
            PresentBench2Core
            GTest::gtest
            GTest::gtest_main
    )
    # Allow CliRejectTests to locate PresentBench2.exe next to the test binary.
    add_dependencies(PresentBench2Tests PresentBench2)
    add_custom_command(
        TARGET PresentBench2Tests
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:PresentBench2>"
            "$<TARGET_FILE_DIR:PresentBench2Tests>"
        VERBATIM
    )
    if(PMON_PRESENTBENCH2_HAS_XESS_FG)
        foreach(_pb2_dll IN LISTS PMON_PRESENTBENCH2_XESS_FG_RUNTIME_DLLS)
            add_custom_command(
                TARGET PresentBench2Tests
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_pb2_dll}"
                    "$<TARGET_FILE_DIR:PresentBench2Tests>"
                VERBATIM
            )
        endforeach()
    endif()
    set_target_properties(PresentBench2Tests PROPERTIES FOLDER "Tests")
    include(GoogleTest)
    gtest_discover_tests(
        PresentBench2Tests
        DISCOVERY_MODE PRE_TEST
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:PresentBench2Tests>"
    )

    foreach(_pb2_api IN ITEMS dx9 dx11 dx12)
        add_test(
            NAME "PresentBench2.BackendSmoke.${_pb2_api}"
            COMMAND PresentBench2
                "--${_pb2_api}"
                --frame-count 3
                --frame-sleep 0
                --no-emit-provider-markers
        )
        set_tests_properties(
            "PresentBench2.BackendSmoke.${_pb2_api}"
            PROPERTIES LABELS "gpu"
        )
    endforeach()
    add_test(
        NAME PresentBench2.BackendSmoke.vk
        COMMAND PresentBench2
            --vk
            --vk-mode FifoKhr
            --frame-count 3
            --frame-sleep 0
            --no-emit-provider-markers
    )
    set_tests_properties(PresentBench2.BackendSmoke.vk PROPERTIES LABELS "gpu")

    if(PMON_PRESENTBENCH2_HAS_LOCAL_VULKAN_VALIDATION_LAYERS)
        add_test(
            NAME PresentBench2.VulkanValidation.MultipleClears
            COMMAND PresentBench2
                --vk
                --enable-debug-layer
                --vk-mode FifoKhr
                --frame-count 10
                --frame-sleep 0
                --clears-per-frame 4
                --no-emit-provider-markers
        )
        set_tests_properties(
            PresentBench2.VulkanValidation.MultipleClears
            PROPERTIES
                ENVIRONMENT
                    "VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT"
                FAIL_REGULAR_EXPRESSION
                    "Validation Error;SYNC-HAZARD;VUID-"
                LABELS
                    "gpu;validation"
        )
    endif()

    find_program(PMON_PRESENTBENCH2_GLSLC glslc)
    if(PMON_PRESENTBENCH2_GLSLC AND PMON_PYTHON_EXECUTABLE)
        add_test(
            NAME PresentBench2.SpirvUpToDate
            COMMAND
                "${PMON_PYTHON_EXECUTABLE}"
                "${PROJECT_SOURCE_DIR}/Scripts/generate_presentbench2_spirv.py"
                --glslc "${PMON_PRESENTBENCH2_GLSLC}"
                --check
        )
    endif()
endif()
