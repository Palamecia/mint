project(debug)

add_library(debug OBJECT)

target_sources(
	debug
	PUBLIC
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
	set_target_properties(debug PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
mint_install_headers($<TARGET_PROPERTY:debug,INTERFACE_SOURCES> SUBDIR ${PROJECT_NAME})
