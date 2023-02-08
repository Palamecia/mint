project(system)

set(${PROJECT_NAME}_HEADERS
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/assert.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/bufferstream.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/datastream.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/errno.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/error.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/filestream.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/filesystem.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/mintsystemerror.hpp
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/malloc.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/plugin.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/poolallocator.hpp
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/stdio.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/terminal.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/utf8.h
)

set(${PROJECT_NAME}_SOURCES
	assert.cpp
	bufferstream.cpp
	datastream.cpp
	errno.cpp
	error.cpp
	filestream.cpp
	filesystem.cpp
	malloc.cpp
	plugin.cpp
	stdio.cpp
	terminal.cpp
	utf8.cpp
)

if (WIN32)
	set(${PROJECT_NAME}_SYSTEM_HEADERS
		win32/terminal.h
	)
    set(${PROJECT_NAME}_SYSTEM_SOURCES
		win32/terminal.cpp
	)
else()
	set(${PROJECT_NAME}_SYSTEM_HEADERS
		unix/terminal.h
	)
    set(${PROJECT_NAME}_SYSTEM_SOURCES
		unix/terminal.cpp
	)
endif()

source_group("Header Files System" FILES ${${PROJECT_NAME}_SYSTEM_HEADERS})
source_group("Source Files System" FILES ${${PROJECT_NAME}_SYSTEM_SOURCES})

add_library(${PROJECT_NAME}
	OBJECT
	${${PROJECT_NAME}_SYSTEM_HEADERS}
	${${PROJECT_NAME}_SYSTEM_SOURCES}
	${${PROJECT_NAME}_HEADERS}
	${${PROJECT_NAME}_SOURCES}
)

if (UNIX)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
install_headers(${${PROJECT_NAME}_HEADERS} SUBDIR ${PROJECT_NAME})
