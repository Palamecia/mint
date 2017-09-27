project(ast)

set(${PROJECT_NAME}_HDR
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/abstractsyntaxtree.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/cursor.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/debuginfos.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/instruction.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/module.h
)

set(${PROJECT_NAME}_SRC
    abstractsyntaxtree.cpp
	cursor.cpp
    debuginfos.cpp
    instruction.cpp
    module.cpp
)

add_library(${PROJECT_NAME}
    OBJECT
    ${${PROJECT_NAME}_HDR}
    ${${PROJECT_NAME}_SRC}
)

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
