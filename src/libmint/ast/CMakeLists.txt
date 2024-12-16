project(ast)

add_library(ast OBJECT)

target_sources(
	ast
	PUBLIC
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/abstractsyntaxtree.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/asttools.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/classregister.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/cursor.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/fileprinter.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/module.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/node.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/printer.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/savedstate.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/symbol.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/symbolmapping.hpp
	PRIVATE
	abstractsyntaxtree.cpp
	asttools.cpp
	classregister.cpp
	cursor.cpp
	fileprinter.cpp
	module.cpp
	node.cpp
	printer.cpp
	savedstate.cpp
	symbol.cpp
	threadentrypoint.cpp
	threadentrypoint.h
)

if (UNIX)
	set_target_properties(ast PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
mint_install_headers($<TARGET_PROPERTY:ast,INTERFACE_SOURCES> SUBDIR ${PROJECT_NAME})
