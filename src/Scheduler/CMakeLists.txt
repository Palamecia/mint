project(Scheduler)

set(${PROJECT_NAME}_HDR
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/process.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/processor.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/scheduler.h
)

set(${PROJECT_NAME}_SRC
    process.cpp
    processor.cpp
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
