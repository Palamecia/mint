project(module-codec)

set(${PROJECT_NAME}_LIB
	iconv.mn
	mimetypes.mn
)

add_custom_target(${PROJECT_NAME}
	SOURCES
	${${PROJECT_NAME}_LIB}
)

# Install
mint_install_modules(${${PROJECT_NAME}_LIB} SUBDIR codec)
