project(compiler)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

option(MINT_DUMP_ASSEMBLY "Dump compiled assembly on debug" ON)
if (MINT_DUMP_ASSEMBLY)
	add_definitions("-DMINT_DUMP_ASSEMBLY")
endif()

set(COMPILER_YY
	${CMAKE_CURRENT_SOURCE_DIR}/parser.y
)

set(COMPILER_HPP
	${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
)

find_package(BISON REQUIRED)
add_custom_command(
	OUTPUT ${COMPILER_HPP}
	DEPENDS ${COMPILER_YY}
	COMMAND ${BISON_EXECUTABLE}
	ARGS -y ${COMPILER_YY}
	     -o ${COMPILER_HPP}
		 -L C++
		 -v
	COMMENT "Generating file ${compiler_HPP}"
)

add_library(compiler OBJECT)

target_sources(
	compiler
	PUBLIC
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/buildtool.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/compiler.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/lexicalhandler.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/lexer.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/token.h
	PRIVATE
	${COMPILER_HPP}
	${COMPILER_YY}
	block.cpp
	block.h
	branch.cpp
	branch.h
	buildtool.cpp
	casetable.cpp
	casetable.h
	catchcontext.cpp
	catchcontext.h
	compiler.cpp
	context.cpp
	context.h
	lexer.cpp
	lexicalhandler.cpp
)

if (UNIX)
	set_target_properties(compiler PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
mint_install_headers($<TARGET_PROPERTY:compiler,INTERFACE_SOURCES> SUBDIR ${PROJECT_NAME})
