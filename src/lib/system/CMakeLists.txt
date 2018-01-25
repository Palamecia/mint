project(mint-system)

set(${PROJECT_NAME}_SRC
	file.cpp
	process.cpp
	thread.cpp
)

set(${PROJECT_NAME}_LIB
	${MINT_MODULES_DIR}/system/file.mn
	${MINT_MODULES_DIR}/system/process.mn
	${MINT_MODULES_DIR}/system/thread.mn
)

add_library(${PROJECT_NAME}
    MODULE
	${${PROJECT_NAME}_SRC}
)

target_link_libraries(${PROJECT_NAME} mint_library)

# Install
install_library(${PROJECT_NAME} SUBDIR mint)
install_modules(${${PROJECT_NAME}_LIB} SUBDIR system)
