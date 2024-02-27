project(debug)

set(${PROJECT_NAME}_HEADERS
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/cursordebugger.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/debuginfo.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/debuginterface.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/debugtool.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/lineinfo.h
)

set(${PROJECT_NAME}_SOURCES
	cursordebugger.cpp
	debuginfo.cpp
	debuginterface.cpp
	debugtool.cpp
	lineinfo.cpp
	threadcontext.hpp
)

add_library(${PROJECT_NAME}
	OBJECT
	${${PROJECT_NAME}_HEADERS}
	${${PROJECT_NAME}_SOURCES}
)

if (UNIX)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
mint_install_headers(${${PROJECT_NAME}_HEADERS} SUBDIR ${PROJECT_NAME})
