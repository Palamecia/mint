project(AbstractSyntaxTree)

set(${PROJECT_NAME}_HDR
    abstractsyntaxtree.h
    instruction.h
    module.h
)

set(${PROJECT_NAME}_SRC
    abstractsyntaxtree.cpp
    instruction.cpp
    module.cpp
)

add_library(${PROJECT_NAME}
    ${${PROJECT_NAME}_HDR}
    ${${PROJECT_NAME}_SRC}
)

target_link_libraries(${PROJECT_NAME}
    Memory
)
