project(Scheduler)

set(${PROJECT_NAME}_HDR
    process.h
    scheduler.h
)

set(${PROJECT_NAME}_SRC
    process.cpp
    scheduler.cpp
)

add_library(${PROJECT_NAME}
    ${${PROJECT_NAME}_HDR}
    ${${PROJECT_NAME}_SRC}
)

target_link_libraries(${PROJECT_NAME}
    AbstractSyntaxTree
    Compiler
    System
)

add_dependencies(${PROJECT_NAME} AbstractSyntaxTree)
