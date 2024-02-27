project(scheduler)

set(${PROJECT_NAME}_HEADERS
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/destructor.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/exception.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/generator.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/inputstream.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/output.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/process.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/processor.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/scheduler.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/threadpool.h
)

set(${PROJECT_NAME}_SOURCES
	bracematcher.cpp
	bracematcher.h
	completer.cpp
	completer.h
	destructor.cpp
	exception.cpp
	generator.cpp
	highlighter.cpp
	highlighter.h
	inputstream.cpp
	output.cpp
	process.cpp
	processor.cpp
	scheduler.cpp
	threadpool.cpp
)

add_library(${PROJECT_NAME}
	OBJECT
	${${PROJECT_NAME}_HEADERS}
	${${PROJECT_NAME}_SOURCES}
)

if (UNIX)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

add_dependencies(${PROJECT_NAME} ast compiler memory system)

# Install
mint_install_headers(${${PROJECT_NAME}_HEADERS} SUBDIR ${PROJECT_NAME})
