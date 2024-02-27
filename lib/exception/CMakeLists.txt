project(module-exception)

set(${PROJECT_NAME}_LIB
	container.mn
	exception.mn
	network.mn
	system.mn
)

add_custom_target(${PROJECT_NAME}
	SOURCES
	${${PROJECT_NAME}_LIB}
)

# Install
mint_install_modules(${${PROJECT_NAME}_LIB} SUBDIR exception)
