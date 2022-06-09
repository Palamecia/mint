project(system)

set(${PROJECT_NAME}_HDR
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/assert.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/bufferstream.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/datastream.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/errno.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/error.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/filestream.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/filesystem.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/inputstream.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/mintsystemerror.hpp
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/optional.hpp
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/plugin.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/poolallocator.hpp
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/stdio.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/terminal.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/utf8iterator.h
)

set(${PROJECT_NAME}_SRC
	assert.cpp
	bufferstream.cpp
	datastream.cpp
	errno.cpp
	error.cpp
	filestream.cpp
	filesystem.cpp
	inputstream.cpp
	plugin.cpp
	stdio.cpp
	terminal.cpp
	utf8iterator.cpp
)

if (WIN32)
	set(${PROJECT_NAME}_SRC
		${${PROJECT_NAME}_SRC}
		winterminal.cpp
	)
endif()

add_library(${PROJECT_NAME}
	OBJECT
	${${PROJECT_NAME}_SRC}
)

if (UNIX)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
install_headers(${${PROJECT_NAME}_HDR} SUBDIR ${PROJECT_NAME})
