project(system)

add_library(system OBJECT)

if (WIN32)
	set(SYSTEM_SPECIFIC_HEADERS
		win32/globalsid.h
		win32/pipe.h
		win32/terminal.h
	)
    set(SYSTEM_SPECIFIC_SOURCES
		win32/globalsid.cpp
		win32/pipe.cpp
		win32/terminal.cpp
	)
else()
	set(SYSTEM_SPECIFIC_HEADERS
		unix/terminal.h
	)
    set(SYSTEM_SPECIFIC_SOURCES
		unix/terminal.cpp
	)
endif()

source_group("Header Files System" FILES ${SYSTEM_SPECIFIC_HEADERS})
source_group("Source Files System" FILES ${SYSTEM_SPECIFIC_SOURCES})

target_sources(
	system
	PUBLIC
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/assert.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/bufferstream.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/datastream.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/errno.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/error.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/filestream.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/filesystem.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/mintsystemerror.hpp
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/pipe.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/plugin.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/poolallocator.hpp
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/stdio.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/string.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/terminal.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/utf8.h
	PRIVATE
	${SYSTEM_SPECIFIC_HEADERS}
	${SYSTEM_SPECIFIC_SOURCES}
	assert.cpp
	bufferstream.cpp
	datastream.cpp
	errno.cpp
	error.cpp
	filestream.cpp
	filesystem.cpp
	pipe.cpp
	plugin.cpp
	stdio.cpp
	string.cpp
	terminal.cpp
	utf8.cpp
)

if (UNIX)
	set_target_properties(system PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
mint_install_headers($<TARGET_PROPERTY:system,INTERFACE_SOURCES> SUBDIR ${PROJECT_NAME})
