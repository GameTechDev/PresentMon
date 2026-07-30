include_guard(GLOBAL)

function(pmon_configure_vcpkg)
    option(PMON_USE_VCPKG "Use the pinned vcpkg manifest" ON)
    option(
        PMON_AUTO_RESTORE_VCPKG
        "Allow vcpkg to restore manifest dependencies during CMake configure"
        OFF
    )
    if(NOT PMON_USE_VCPKG)
        return()
    endif()

    if(NOT CMAKE_TOOLCHAIN_FILE)
        set(vcpkg_root "")
        if(DEFINED ENV{VCPKG_ROOT})
            set(vcpkg_root "$ENV{VCPKG_ROOT}")
        elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
            set(vcpkg_root "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg")
        else()
            set(vswhere "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer/vswhere.exe")
            if(EXISTS "${vswhere}")
                execute_process(
                    COMMAND
                        "${vswhere}"
                        -latest
                        -products *
                        -requires Microsoft.Component.MSBuild
                        -property installationPath
                    OUTPUT_VARIABLE visual_studio_path
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET
                )
                if(visual_studio_path)
                    set(vcpkg_root "${visual_studio_path}/VC/vcpkg")
                endif()
            endif()
        endif()

        set(vcpkg_toolchain "${vcpkg_root}/scripts/buildsystems/vcpkg.cmake")
        if(NOT EXISTS "${vcpkg_toolchain}")
            message(
                FATAL_ERROR
                "PMON_USE_VCPKG=ON, but vcpkg was not found. Set VCPKG_ROOT, "
                "provide a repository-local vcpkg checkout, or disable PMON_USE_VCPKG."
            )
        endif()

        set(
            CMAKE_TOOLCHAIN_FILE
            "${vcpkg_toolchain}"
            CACHE FILEPATH
            "vcpkg toolchain file"
        )
    endif()
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
        set(default_target_triplet "x86-windows-static")
    else()
        set(default_target_triplet "x64-windows-static")
    endif()
    set(VCPKG_TARGET_TRIPLET "${default_target_triplet}" CACHE STRING "vcpkg target triplet")
    set(VCPKG_HOST_TRIPLET "x64-windows-static" CACHE STRING "vcpkg host triplet")
    set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "vcpkg manifest directory")
    set(
        VCPKG_MANIFEST_INSTALL
        "${PMON_AUTO_RESTORE_VCPKG}"
        CACHE BOOL
        "Restore vcpkg manifest dependencies during configure"
    )
    set(
        VCPKG_INSTALLED_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/build/ThirdParty/vcpkg_installed/${default_target_triplet}"
        CACHE PATH
        "Shared per-architecture vcpkg installation root"
    )
    set(
        ENV{VCPKG_DOWNLOADS}
        "${CMAKE_CURRENT_SOURCE_DIR}/build/ThirdParty/vcpkg-downloads"
    )
endfunction()
