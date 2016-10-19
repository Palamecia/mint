project(mint)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

set(MINT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(MINT_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_INSTALL_PREFIX "/")

include_directories(${MINT_INCLUDE_DIR})

add_subdirectory(src/AbstractSyntaxTree)
add_subdirectory(src/Compiler)
add_subdirectory(src/Memory)
add_subdirectory(src/Scheduler)
add_subdirectory(src/System)

set(${PROJECT_NAME}_SRC
    src/main.cpp
)

add_executable(${PROJECT_NAME}
    ${${PROJECT_NAME}_SRC}
)

target_link_libraries(${PROJECT_NAME}
    Scheduler
)

add_dependencies(${PROJECT_NAME} Scheduler)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(DIRECTORY ${MINT_LIBRARY_DIR}/ DESTINATION usr/lib/mint)
install(DIRECTORY ${MINT_INCLUDE_DIR}/ DESTINATION usr/include/mint)
