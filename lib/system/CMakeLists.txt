project(module-system)

set(${PROJECT_NAME}_LIB
	date.mn
	directory.mn
	file.mn
	locale.mn
	mutex.mn
	process.mn
	terminal.mn
	thread.mn
	timezone.mn
)

add_custom_target(${PROJECT_NAME}
	SOURCES
	${${PROJECT_NAME}_LIB}
)

# Install
install_modules(${${PROJECT_NAME}_LIB} SUBDIR system)
