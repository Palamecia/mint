project(AbstractSyntaxTree)

set(${PROJECT_NAME}_HDR
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/abstractsyntaxtree.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/instruction.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/module.h
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
