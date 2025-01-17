cmake_minimum_required(VERSION 3.23)
project(mint)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/share/cmake)

include(mint-flags)
include(mint-install)

# Version configuration
set(MINT_VERSION_MAJOR 0)
set(MINT_VERSION_MINOR 0)
set(MINT_VERSION_PATCH 0)
set(MINT_VERSION ${MINT_VERSION_MAJOR}.${MINT_VERSION_MINOR}.${MINT_VERSION_PATCH})
add_definitions("-DMINT_VERSION=${MINT_VERSION}")

set(MINT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/mint)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Project subdirectories
add_subdirectory(src)
add_subdirectory(lib)
add_subdirectory(bin)
add_subdirectory(share)

# Test
find_package(GTest QUIET)
if (GTEST_FOUND)
	include(CTest)
	if (BUILD_TESTING)
		message(STATUS "Tests enabled")
		add_subdirectory(tests)
	endif()
endif()

# Install
mint_install_headers(${MINT_INCLUDE_DIR}/config.h)
