cmake_minimum_required(VERSION 3.1)

project(mint)

# Version configuration
set(MINT_VERSION_MAJOR 0)
set(MINT_VERSION_MINOR 0)
set(MINT_VERSION_PATCH 0)
set(MINT_VERSION ${MINT_VERSION_MAJOR}.${MINT_VERSION_MINOR}.${MINT_VERSION_PATCH})
add_definitions("-DMINT_VERSION=${MINT_VERSION}")

# Path configuration
set(MINT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(MINT_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if (UNIX)
    set(CMAKE_INSTALL_PREFIX "/")
    set(MINT_INSTALL_RUNTIME_DIR "bin")
    set(MINT_INSTALL_LIBRARY_DIR "lib64")
    set(MINT_INSTALL_HEADERS_DIR "usr/include")
else()
    set(CMAKE_INSTALL_PREFIX "C:/Program Files/")
    set(MINT_INSTALL_RUNTIME_DIR "bin")
    set(MINT_INSTALL_LIBRARY_DIR "bin")
    set(MINT_INSTALL_HEADERS_DIR "include")
endif()

# C++ flags
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

if (CMAKE_BUILD_TYPE MATCHES "Sanitize=address")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

if (CMAKE_BUILD_TYPE MATCHES "Sanitize=leak")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
endif()


# Project subdirectories
include_directories(include)
add_subdirectory(src)

# Test
find_package(GTest QUIET)
if (GTEST_FOUND)
	message(STATUS "Tests enabled")
	add_subdirectory(tests)
endif()

# Install
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ DESTINATION ${MINT_INSTALL_LIBRARY_DIR}/${PROJECT_NAME})
install(DIRECTORY ${MINT_LIBRARY_DIR}/ DESTINATION ${MINT_INSTALL_LIBRARY_DIR}/${PROJECT_NAME})
install(DIRECTORY ${MINT_INCLUDE_DIR}/ DESTINATION ${MINT_INSTALL_HEADERS_DIR}/${PROJECT_NAME})
