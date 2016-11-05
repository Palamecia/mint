cmake_minimum_required(VERSION 2.8)

project(mint)

set(MINT_VERSION_MAJOR 0)
set(MINT_VERSION_MINOR 0)
set(MINT_VERSION_PATCH 0)
set(MINT_VERSION ${MINT_VERSION_MAJOR}.${MINT_VERSION_MINOR}.${MINT_VERSION_PATCH})

set(MINT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(MINT_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/lib)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_INSTALL_PREFIX "/")

include_directories(${MINT_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

add_subdirectory(src/ast)
add_subdirectory(src/compiler)
add_subdirectory(src/memory)
add_subdirectory(src/scheduler)
add_subdirectory(src/system)

add_subdirectory(src/lib)

add_library(${PROJECT_NAME}-${MINT_VERSION}
    SHARED
    $<TARGET_OBJECTS:ast>
    $<TARGET_OBJECTS:compiler>
    $<TARGET_OBJECTS:memory>
    $<TARGET_OBJECTS:scheduler>
    $<TARGET_OBJECTS:system>
)

if (UNIX)
    target_link_libraries(${PROJECT_NAME}-${MINT_VERSION} dl)
endif()
add_dependencies(${PROJECT_NAME}-${MINT_VERSION} scheduler)

add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-${MINT_VERSION})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(TARGETS ${PROJECT_NAME}-${MINT_VERSION} LIBRARY DESTINATION lib64)
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ DESTINATION lib64/mint)
install(DIRECTORY ${MINT_LIBRARY_DIR}/ DESTINATION lib64/mint)
install(DIRECTORY ${MINT_INCLUDE_DIR}/ DESTINATION usr/include/mint)
