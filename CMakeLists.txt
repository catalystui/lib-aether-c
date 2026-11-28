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
    set(CATSYLIB_PLATFORM_SOURCE src/catsylib.win32.cpp)
elseif(TARGET_PLATFORM_LINUX OR TARGET_PLATFORM_DARWIN)
    set(CATSYLIB_PLATFORM_SOURCE src/catsylib.posix.cpp)
else()
    message(FATAL_ERROR "CATSYLIB has no implementation for this target platform.")
endif()

add_library(catsylib_shared SHARED)
add_library(catsylib_static STATIC)
configure_target(
    NAME catsy
    TARGETS catsylib_shared catsylib_static
    SOURCES
        src/catsylib.cpp
        src/catsylib.internal.hpp
        ${CATSYLIB_PLATFORM_SOURCE}
)
foreach(TARGET_NAME catsylib_shared catsylib_static)
    target_include_directories(${TARGET_NAME} PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
    )
    target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_link_libraries(${TARGET_NAME} PUBLIC Catalyst::catalyst)
    if(TARGET_PLATFORM_WIN32)
        target_link_libraries(${TARGET_NAME} PRIVATE advapi32)
    endif()
endforeach()
target_compile_definitions(catsylib_shared PRIVATE CATSYLIB_EXPORTS INTERFACE CATSYLIB_USE_DLL)
target_link_libraries(catsylib_static PRIVATE Catalyst::catmm_static Catalyst::catte_static)
target_link_libraries(catsylib_shared PRIVATE Catalyst::catmm_shared Catalyst::catte_shared)

add_library(Catalyst::catsy_static ALIAS catsylib_static)
add_library(Catalyst::catsy_shared ALIAS catsylib_shared)

# An installed Core may be outside Aether's runtime directory. Permit CMake
# to resolve its shared library location during the build, retaining the
# existing relative install RPATH for distribution.
get_target_property(CORE_IMPORTED Catalyst::catmm_shared IMPORTED)
if(CORE_IMPORTED AND (TARGET_PLATFORM_LINUX OR TARGET_PLATFORM_DARWIN))
    set_target_properties(catsylib_shared PROPERTIES
        BUILD_WITH_INSTALL_RPATH FALSE
        BUILD_RPATH_USE_ORIGIN TRUE
    )
endif()

# Native device and I/O modules consume the same Core targets as CATSYLIB.
foreach(MODULE catdv catin catot)
    set(MODULE_NAME "${MODULE}lib")
    if(TARGET_PLATFORM_LINUX)
        set(MODULE_SOURCE "src/${MODULE_NAME}.linux.cpp")
    elseif(TARGET_PLATFORM_WIN32)
        set(MODULE_SOURCE "src/${MODULE_NAME}.win32.cpp")
    elseif(TARGET_PLATFORM_DARWIN)
        set(MODULE_SOURCE "src/${MODULE_NAME}.darwin.cpp")
    else()
        set(MODULE_SOURCE "src/${MODULE_NAME}.cpp")
    endif()
    if(MODULE STREQUAL "catdv" AND (TARGET_PLATFORM_WIN32 OR TARGET_PLATFORM_DARWIN))
        list(APPEND MODULE_SOURCE src/catdvlib.native.cpp)
    endif()
    add_library(${MODULE_NAME}_shared SHARED)
    add_library(${MODULE_NAME}_static STATIC)
    configure_target(
        NAME ${MODULE}
        TARGETS ${MODULE_NAME}_shared ${MODULE_NAME}_static
        SOURCES ${MODULE_SOURCE}
    )
    string(TOUPPER "${MODULE_NAME}" MODULE_UPPER)
    target_compile_definitions(${MODULE_NAME}_shared PRIVATE ${MODULE_UPPER}_EXPORTS INTERFACE ${MODULE_UPPER}_USE_DLL)
    foreach(KIND shared static)
        set(TARGET_NAME "${MODULE_NAME}_${KIND}")
        target_include_directories(${TARGET_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
        target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
        target_link_libraries(${TARGET_NAME} PUBLIC Catalyst::catalyst)
        if(TARGET_PLATFORM_LINUX OR TARGET_PLATFORM_WIN32 OR TARGET_PLATFORM_DARWIN)
            target_link_libraries(${TARGET_NAME} PRIVATE Catalyst::catmm_${KIND} Catalyst::catte_${KIND})
        endif()
        if(TARGET_PLATFORM_WIN32)
            target_link_libraries(${TARGET_NAME} PRIVATE hid)
            if(MODULE STREQUAL "catdv")
                target_link_libraries(${TARGET_NAME} PRIVATE setupapi)
            endif()
        elseif(TARGET_PLATFORM_DARWIN)
            target_link_libraries(${TARGET_NAME} PRIVATE "-framework IOKit" "-framework CoreFoundation")
        endif()
        if(NOT MODULE STREQUAL "catdv")
            target_link_libraries(${TARGET_NAME} PUBLIC Catalyst::catdv_${KIND})
        endif()
        add_library(Catalyst::${MODULE}_${KIND} ALIAS ${TARGET_NAME})
    endforeach()
    if(CORE_IMPORTED AND (TARGET_PLATFORM_LINUX OR TARGET_PLATFORM_DARWIN))
        set_target_properties(${MODULE_NAME}_shared PROPERTIES
            BUILD_WITH_INSTALL_RPATH FALSE
            BUILD_RPATH_USE_ORIGIN TRUE
        )
    endif()
endforeach()

# Define standalone verification and performance executables.
if(CATALYST_AETHER_BUILD_TESTS)
    define_test_executables(
        STATIC_DEPENDS
            catsylib_static
        SHARED_DEPENDS
            catsylib_shared
    )
endif()
