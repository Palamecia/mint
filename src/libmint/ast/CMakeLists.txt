project(ast)

set(${PROJECT_NAME}_HDR
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/abstractsyntaxtree.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/classregister.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/cursor.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/module.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/node.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/savedstate.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/symbol.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/symbolmapping.hpp
)

set(${PROJECT_NAME}_SRC
	abstractsyntaxtree.cpp
	classregister.cpp
	cursor.cpp
	module.cpp
	node.cpp
	savedstate.cpp
	symbol.cpp
	threadentrypoint.cpp
	threadentrypoint.h
)

add_library(${PROJECT_NAME}
	OBJECT
	${${PROJECT_NAME}_SRC}
)

if (UNIX)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
install_headers(${${PROJECT_NAME}_HDR} SUBDIR ${PROJECT_NAME})
