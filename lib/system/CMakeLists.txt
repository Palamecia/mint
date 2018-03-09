project(module-system)

set(${PROJECT_NAME}_LIB
	directory.mn
	file.mn
	process.mn
	thread.mn
)

add_custom_target(${PROJECT_NAME}
	SOURCES
	${${PROJECT_NAME}_LIB}
)

# Install
install_modules(${${PROJECT_NAME}_LIB} SUBDIR system)
