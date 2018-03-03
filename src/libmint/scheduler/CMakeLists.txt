project(scheduler)

set(${PROJECT_NAME}_HDR
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/destructor.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/process.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/processor.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/scheduler.h
)

set(${PROJECT_NAME}_SRC
	destructor.cpp
	process.cpp
	processor.cpp
	scheduler.cpp
)

add_library(${PROJECT_NAME}
	OBJECT
	${${PROJECT_NAME}_SRC}
)

if (UNIX)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

add_dependencies(${PROJECT_NAME} ast compiler memory system)

# Install
install_headers(${${PROJECT_NAME}_HDR} SUBDIR ${PROJECT_NAME})
