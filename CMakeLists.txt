cmake_minimum_required(VERSION 4.3.0)
project(aether C CXX)
if(PROJECT_IS_TOP_LEVEL)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()
include("${PROJECT_SOURCE_DIR}/cmake/CMakeInit.cmake" REQUIRED)
metadata("aether" 1 0 0 "alpha.1" "Cross-platform C ABI library for native system-level bindings and operating system functionality.")
configure("${TARGET_VENDOR}" "${TARGET_SYSTEM}" "${TARGET_ARCHITECTURE}")
define("cmake/CMakeConfig.h" "cmakecfg.h")

# DARWIN platform specifics
if(TARGET_PLATFORM_DARWIN)
    # Enable Objective-C and Objective-C++
    enable_language(OBJC OBJCXX)
endif()

# Reuse a parent's Core targets; otherwise select one pinned source revision
# or an explicitly requested installed package for the entire build.
option(CATALYST_AETHER_USE_INSTALLED_CORE "Consume an installed CatalystCore package" OFF)
set(CATALYST_CORE_REVISION "f11851fcfb6d4ddfaa802ee92ddc01bba69f810f" CACHE STRING "Core source revision")
if(NOT TARGET Catalyst::catalyst)
    if(CATALYST_AETHER_USE_INSTALLED_CORE)
        find_package(CatalystCore 1.0.0 EXACT CONFIG REQUIRED GLOBAL)
    else()
        include(FetchContent)
        FetchContent_Declare(
            CatalystCore
            GIT_REPOSITORY https://github.com/catalystui/lib-core-c.git
            GIT_TAG "${CATALYST_CORE_REVISION}"
            GIT_SUBMODULES cmake
        )
        FetchContent_MakeAvailable(CatalystCore)
    endif()
endif()
foreach(CORE_TARGET Catalyst::catalyst Catalyst::catmm_static Catalyst::catmm_shared Catalyst::catte_static Catalyst::catte_shared)
    if(NOT TARGET ${CORE_TARGET})
        message(FATAL_ERROR "Aether requires the Core target ${CORE_TARGET}.")
    endif()
endforeach()

option(CATALYST_AETHER_BUILD_TESTS "Build Aether's standalone verification programs" ${PROJECT_IS_TOP_LEVEL})

if(TARGET_PLATFORM_WIN32)
    set(ATHIDLIB_PLATFORM_SOURCE src/athidlib.win32.cpp)
elseif(TARGET_PLATFORM_LINUX OR TARGET_PLATFORM_DARWIN)
    set(ATHIDLIB_PLATFORM_SOURCE src/athidlib.posix.cpp)
else()
    message(FATAL_ERROR "ATHIDLIB has no implementation for this target platform.")
endif()

add_library(athidlib_shared SHARED)
add_library(athidlib_static STATIC)
configure_target(
    NAME athid
    TARGETS athidlib_shared athidlib_static
    SOURCES
        src/athidlib.cpp
        src/athidlib.internal.hpp
        ${ATHIDLIB_PLATFORM_SOURCE}
)
foreach(TARGET_NAME athidlib_shared athidlib_static)
    target_include_directories(${TARGET_NAME} PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
    )
    target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_link_libraries(${TARGET_NAME} PUBLIC Catalyst::catalyst)
    if(TARGET_PLATFORM_WIN32)
        target_link_libraries(${TARGET_NAME} PRIVATE advapi32)
    endif()
endforeach()
target_compile_definitions(athidlib_shared PRIVATE ATHIDLIB_EXPORTS INTERFACE ATHIDLIB_USE_DLL)
target_link_libraries(athidlib_static PRIVATE Catalyst::catmm_static Catalyst::catte_static)
target_link_libraries(athidlib_shared PRIVATE Catalyst::catmm_shared Catalyst::catte_shared)

add_library(Catalyst::athid_static ALIAS athidlib_static)
add_library(Catalyst::athid_shared ALIAS athidlib_shared)

# An installed Core may be outside Aether's runtime directory. Permit CMake
# to resolve its shared library location during the build, retaining the
# existing relative install RPATH for distribution.
get_target_property(CORE_IMPORTED Catalyst::catmm_shared IMPORTED)
if(CORE_IMPORTED AND (TARGET_PLATFORM_LINUX OR TARGET_PLATFORM_DARWIN))
    set_target_properties(athidlib_shared PROPERTIES
        BUILD_WITH_INSTALL_RPATH FALSE
        BUILD_RPATH_USE_ORIGIN TRUE
    )
endif()

# Native device discovery consumes the same Core targets as ATHIDLIB.
if(TARGET_PLATFORM_LINUX)
    set(MODULE_SOURCE "src/athdvlib.linux.cpp")
elseif(TARGET_PLATFORM_WIN32)
    set(MODULE_SOURCE "src/athdvlib.win32.cpp")
elseif(TARGET_PLATFORM_DARWIN)
    set(MODULE_SOURCE "src/athdvlib.darwin.cpp")
else()
    set(MODULE_SOURCE "src/athdvlib.cpp")
endif()
if(TARGET_PLATFORM_WIN32 OR TARGET_PLATFORM_DARWIN)
    list(APPEND MODULE_SOURCE src/athdvlib.native.cpp)
endif()
add_library(athdvlib_shared SHARED)
add_library(athdvlib_static STATIC)
configure_target(
    NAME athdv
    TARGETS athdvlib_shared athdvlib_static
    SOURCES ${MODULE_SOURCE} src/athdvlib.lookup.cpp
)
target_compile_definitions(athdvlib_shared PRIVATE ATHDVLIB_EXPORTS INTERFACE ATHDVLIB_USE_DLL)
foreach(KIND shared static)
    set(TARGET_NAME "athdvlib_${KIND}")
    target_include_directories(${TARGET_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
    target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_link_libraries(${TARGET_NAME} PUBLIC Catalyst::catalyst)
    if(TARGET_PLATFORM_LINUX OR TARGET_PLATFORM_WIN32 OR TARGET_PLATFORM_DARWIN)
        target_link_libraries(${TARGET_NAME} PRIVATE Catalyst::catmm_${KIND} Catalyst::catte_${KIND})
    endif()
    if(TARGET_PLATFORM_WIN32)
        target_link_libraries(${TARGET_NAME} PRIVATE hid setupapi user32 advapi32)
    elseif(TARGET_PLATFORM_DARWIN)
        target_link_libraries(${TARGET_NAME} PRIVATE "-framework IOKit" "-framework CoreFoundation" "-framework CoreGraphics")
    endif()
    add_library(Catalyst::athdv_${KIND} ALIAS ${TARGET_NAME})
endforeach()
if(CORE_IMPORTED AND (TARGET_PLATFORM_LINUX OR TARGET_PLATFORM_DARWIN))
    set_target_properties(athdvlib_shared PROPERTIES
        BUILD_WITH_INSTALL_RPATH FALSE
        BUILD_RPATH_USE_ORIGIN TRUE
    )
endif()

# Define standalone verification and performance executables.
if(CATALYST_AETHER_BUILD_TESTS)
    define_test_executables(
        STATIC_DEPENDS
            athidlib_static
        SHARED_DEPENDS
            athidlib_shared
    )
    target_link_libraries(test_athdvlib PRIVATE Catalyst::athdv_static)
    target_link_libraries(perf_athdvlib PRIVATE Catalyst::athdv_static)
endif()
