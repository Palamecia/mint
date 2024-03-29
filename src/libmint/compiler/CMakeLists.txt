project(compiler)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

option(MINT_DUMP_ASSEMBLY "Dump compiled assembly on debug" ON)
if (MINT_DUMP_ASSEMBLY)
	add_definitions("-DMINT_DUMP_ASSEMBLY")
endif()

set(${PROJECT_NAME}_YY
	${CMAKE_CURRENT_SOURCE_DIR}/parser.y
)

set(${PROJECT_NAME}_HPP
	${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
)

find_package(BISON REQUIRED)
add_custom_command(
	OUTPUT ${${PROJECT_NAME}_HPP}
	DEPENDS ${${PROJECT_NAME}_YY}
	COMMAND ${BISON_EXECUTABLE}
	ARGS -y ${${PROJECT_NAME}_YY}
		 -o ${${PROJECT_NAME}_HPP}
		 -L C++
		 -v
	COMMENT "Generating file ${${PROJECT_NAME}_HPP}"
)

add_library(${PROJECT_NAME} OBJECT)

target_sources(
	${PROJECT_NAME}
	INTERFACE
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/buildtool.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/compiler.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/lexicalhandler.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/lexer.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/token.h
	PRIVATE
	${${PROJECT_NAME}_HPP}
	${${PROJECT_NAME}_YY}
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
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
get_target_property(${PROJECT_NAME}_HEADERS ${PROJECT_NAME} INTERFACE_SOURCES)
mint_install_headers(${${PROJECT_NAME}_HEADERS} SUBDIR ${PROJECT_NAME})
