cmake_minimum_required(VERSION 3.1)

project(mint)

# Version configuration
set(MINT_VERSION_MAJOR 0)
set(MINT_VERSION_MINOR 0)
set(MINT_VERSION_PATCH 0)
set(MINT_VERSION ${MINT_VERSION_MAJOR}.${MINT_VERSION_MINOR}.${MINT_VERSION_PATCH})

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
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

# Project subdirectories
include_directories(${MINT_INCLUDE_DIR})

add_subdirectory(src/ast)
add_subdirectory(src/compiler)
add_subdirectory(src/memory)
add_subdirectory(src/scheduler)
add_subdirectory(src/system)

add_subdirectory(src/lib)

# Target libmint
add_library(lib${PROJECT_NAME}
    SHARED
    $<TARGET_OBJECTS:ast>
    $<TARGET_OBJECTS:compiler>
    $<TARGET_OBJECTS:memory>
    $<TARGET_OBJECTS:scheduler>
    $<TARGET_OBJECTS:system>
)

set_target_properties(lib${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

if (UNIX)
    target_link_libraries(lib${PROJECT_NAME} dl readline)
endif()

add_dependencies(lib${PROJECT_NAME} scheduler)

# Target mint
add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} lib${PROJECT_NAME})

# Install
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${MINT_INSTALL_RUNTIME_DIR})
install(TARGETS lib${PROJECT_NAME} LIBRARY DESTINATION ${MINT_INSTALL_LIBRARY_DIR})
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ DESTINATION ${MINT_INSTALL_LIBRARY_DIR}/${PROJECT_NAME})
install(DIRECTORY ${MINT_LIBRARY_DIR}/ DESTINATION ${MINT_INSTALL_LIBRARY_DIR}/${PROJECT_NAME})
install(DIRECTORY ${MINT_INCLUDE_DIR}/ DESTINATION ${MINT_INSTALL_HEADERS_DIR}/${PROJECT_NAME})

# Test
find_package(GTest QUIET)
if (GTEST_FOUND)
    message(STATUS "Tests enabled")
    add_subdirectory(tests)
endif()
