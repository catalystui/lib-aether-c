cmake_minimum_required(VERSION 4.3.0)
project(aether C CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include_directories("${CMAKE_SOURCE_DIR}/include")
include(${CMAKE_SOURCE_DIR}/cmake/CMakeInit.cmake REQUIRED)
metadata("aether" 1 0 0 "alpha.1" "Cross-platform C ABI library for native system-level bindings and operating system functionality.")
configure("${TARGET_VENDOR}" "${TARGET_SYSTEM}" "${TARGET_ARCHITECTURE}")
define("cmake/CMakeConfig.h" "cmakecfg.h")

# DARWIN platform specifics
if(TARGET_PLATFORM_DARWIN)
    # Enable Objective-C and Objective-C++
    enable_language(OBJC OBJCXX)
endif()

# catsylib uses Core's foundational headers with Aether's generated target configuration.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/core/include/catalyst.h")
    message(FATAL_ERROR "Initialize the Core submodule: git submodule update --init --recursive")
endif()

if(TARGET_PLATFORM_WIN32)
    set(CATSYLIB_PLATFORM_SOURCE src/catsylib.win32.cpp)
elseif(TARGET_PLATFORM_LINUX OR TARGET_PLATFORM_DARWIN)
    set(CATSYLIB_PLATFORM_SOURCE src/catsylib.posix.cpp)
    find_package(Iconv REQUIRED)
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
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/core/include"
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
    )
    if(TARGET_PLATFORM_WIN32)
        target_link_libraries(${TARGET_NAME} PRIVATE advapi32)
    else()
        target_link_libraries(${TARGET_NAME} PRIVATE Iconv::Iconv)
    endif()
endforeach()
target_compile_definitions(catsylib_shared PRIVATE CATSYLIB_EXPORTS INTERFACE CATSYLIB_USE_DLL)

# Native device and I/O modules. Linux uses the pinned Core memory/text
# implementations as private object dependencies (no additional runtime library).
if(TARGET_PLATFORM_LINUX)
    add_library(aether_device_core OBJECT
        core/src/catmmlib.cpp
        core/src/catmmlib.linux.cpp
        core/src/cattelib.cpp
    )
    target_include_directories(aether_device_core PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/core/include"
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
    )
    set_target_properties(aether_device_core PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
    )
endif()

foreach(MODULE catdv catin catot)
    set(MODULE_NAME "${MODULE}lib")
    if(TARGET_PLATFORM_LINUX)
        set(MODULE_SOURCE "src/${MODULE_NAME}.linux.cpp")
    else()
        set(MODULE_SOURCE "src/${MODULE_NAME}.cpp")
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
        target_include_directories(${MODULE_NAME}_${KIND} PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/core/include"
            "${CMAKE_CURRENT_BINARY_DIR}/generated"
        )
        if(TARGET_PLATFORM_LINUX)
            target_sources(${MODULE_NAME}_${KIND} PRIVATE $<TARGET_OBJECTS:aether_device_core>)
        endif()
        if(NOT MODULE STREQUAL "catdv")
            target_link_libraries(${MODULE_NAME}_${KIND} PUBLIC catdvlib_${KIND})
        endif()
    endforeach()
endforeach()

# Define standalone verification and performance executables.
define_test_executables(
    STATIC_DEPENDS
        catsylib_static
    SHARED_DEPENDS
        catsylib_shared
)
