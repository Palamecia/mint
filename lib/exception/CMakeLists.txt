project(module-exception)

set(${PROJECT_NAME}_LIB
	exception.mn
	container.mn
)

add_custom_target(${PROJECT_NAME}
	SOURCES
	${${PROJECT_NAME}_LIB}
)

# Install
install_modules(${${PROJECT_NAME}_LIB} SUBDIR exception)
