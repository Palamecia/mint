project(mint)

cmake_minimum_required(VERSION 3.1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/share/cmake)

include(flags)
include(install)

# Version configuration
set(MINT_VERSION_MAJOR 0)
set(MINT_VERSION_MINOR 0)
set(MINT_VERSION_PATCH 0)
set(MINT_VERSION ${MINT_VERSION_MAJOR}.${MINT_VERSION_MINOR}.${MINT_VERSION_PATCH})
add_definitions("-DMINT_VERSION=${MINT_VERSION}")

set(MINT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# Project subdirectories
include_directories(include)
add_subdirectory(src)
add_subdirectory(lib)
add_subdirectory(share)

# Test
find_package(GTest QUIET)
if (GTEST_FOUND)
	message(STATUS "Tests enabled")
	add_subdirectory(tests)
endif()

# Install
install_headers(${MINT_INCLUDE_DIR}/config.h)
