project(debug)

set(${PROJECT_NAME}_HDR
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/cursordebugger.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/debuginfos.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/debuginterface.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/debugtool.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/lineinfo.h
)

set(${PROJECT_NAME}_SRC
	cursordebugger.cpp
	debuginfos.cpp
	debuginterface.cpp
	debugtool.cpp
	lineinfo.cpp
)

add_library(${PROJECT_NAME}
	OBJECT
	${${PROJECT_NAME}_HDR}
	${${PROJECT_NAME}_SRC}
)

if (UNIX)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
install_headers(${${PROJECT_NAME}_HDR} SUBDIR ${PROJECT_NAME})
