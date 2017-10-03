project(mint-system)

set(${PROJECT_NAME}_SRC
	file.cpp
	process.cpp
	thread.cpp
)

set(${PROJECT_NAME}_LIB
	${MINT_LIBRARY_DIR}/system/file.mn
	${MINT_LIBRARY_DIR}/system/process.mn
	${MINT_LIBRARY_DIR}/system/thread.mn
)

add_library(${PROJECT_NAME}
    MODULE
	${${PROJECT_NAME}_SRC}
	${${PROJECT_NAME}_LIB}
)

target_link_libraries(${PROJECT_NAME} libmint)
