project(mint)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
cmake_minimum_required(VERSION 2.8)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(AbstractSyntaxTree)
add_subdirectory(Compiler)
add_subdirectory(Memory)
add_subdirectory(Scheduler)
add_subdirectory(System)

set(${PROJECT_NAME}_SRC
    main.cpp
)

add_executable(${PROJECT_NAME}
    ${${PROJECT_NAME}_SRC}
)

target_link_libraries(${PROJECT_NAME}
    Scheduler
)

add_dependencies(${PROJECT_NAME} Scheduler)
