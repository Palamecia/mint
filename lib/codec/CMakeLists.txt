project(module-codec)

add_custom_target(${PROJECT_NAME}
	SOURCES
	iconv.mn
	mimetypes.mn
)

# Install
get_target_property(${PROJECT_NAME}_LIB ${PROJECT_NAME} SOURCES)
mint_install_modules(${${PROJECT_NAME}_LIB} SUBDIR codec)
