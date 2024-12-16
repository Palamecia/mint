project(scheduler)

add_library(scheduler OBJECT)

target_sources(
	scheduler
	INTERFACE
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/destructor.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/exception.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/generator.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/inputstream.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/output.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/process.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/processor.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/scheduler.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/threadpool.h
	PRIVATE
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

if (UNIX)
	set_target_properties(scheduler PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

add_dependencies(scheduler ast compiler memory system)

# Install
mint_install_headers($<TARGET_PROPERTY:scheduler,INTERFACE_SOURCES> SUBDIR ${PROJECT_NAME})
