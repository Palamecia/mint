project(debug)

set(${PROJECT_NAME}_HEADERS
)

set(${PROJECT_NAME}_SOURCES
)

add_library(${PROJECT_NAME} OBJECT)

target_sources(
	${PROJECT_NAME}
	INTERFACE
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/cursordebugger.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/debuginfo.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/debuginterface.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/debugtool.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/lineinfo.h
	PRIVATE
	cursordebugger.cpp
	debuginfo.cpp
	debuginterface.cpp
	debugtool.cpp
	lineinfo.cpp
	threadcontext.hpp
)

if (UNIX)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
get_target_property(${PROJECT_NAME}_HEADERS ${PROJECT_NAME} INTERFACE_SOURCES)
mint_install_headers(${${PROJECT_NAME}_HEADERS} SUBDIR ${PROJECT_NAME})
