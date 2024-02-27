project(module-system)

add_custom_target(${PROJECT_NAME}
	SOURCES
	date.mn
	directory.mn
	file.mn
	filesystem.mn
	locale.mn
	os.mn
	process.mn
	terminal.mn
	thread.mn
	timezone.mn
	uri.mn
)

# Install
get_target_property(${PROJECT_NAME}_LIB ${PROJECT_NAME} SOURCES)
mint_install_modules(${${PROJECT_NAME}_LIB} SUBDIR system)
