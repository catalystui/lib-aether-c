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
