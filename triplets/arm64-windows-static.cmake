set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

# Pin the MSVC toolset used to build ARM64 dependencies to v143 to match the
# PlatformToolset the projects link with. Without this, vcpkg builds ARM64
# packages with the newest installed toolset (v145 on VS 2026), whose newer
# vectorized-STL symbols fail to resolve when linked into the v143 binaries.
set(VCPKG_PLATFORM_TOOLSET v143)
