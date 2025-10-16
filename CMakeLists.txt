cmake_minimum_required(VERSION 3.23)
project(mint VERSION 0.0.1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/share/cmake)

include(mint-flags)
include(mint-install)

# Version configuration
add_definitions("-DMINT_VERSION=${CMAKE_PROJECT_VERSION}")

set(MINT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

set(MINT_SHARE_DIR ${CMAKE_SOURCE_DIR}/share)

# Project subdirectories
add_subdirectory(src)
add_subdirectory(lib)
add_subdirectory(bin)
add_subdirectory(share)

# Test
find_package(GTest CONFIG REQUIRED)
include(CTest)
if (BUILD_TESTING)
	message(STATUS "Tests enabled")
	add_subdirectory(tests)
endif()

# Installer
include(CPack)
