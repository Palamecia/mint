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

set(${PROJECT_NAME}_HEADERS
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/buildtool.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/compiler.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/lexicalhandler.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/lexer.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/token.h
)

set(${PROJECT_NAME}_SOURCES
	${${PROJECT_NAME}_HPP}
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
	lexicalhandler.cpp
	lexer.cpp
)

add_library(${PROJECT_NAME}
	OBJECT
	${${PROJECT_NAME}_HEADERS}
	${${PROJECT_NAME}_SOURCES}
	${${PROJECT_NAME}_YY}
)

if (UNIX)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
mint_install_headers(${${PROJECT_NAME}_HEADERS} SUBDIR ${PROJECT_NAME})
