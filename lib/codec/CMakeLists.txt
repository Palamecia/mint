project(module-codec)

add_custom_target(${PROJECT_NAME}
	SOURCES
	iconv.mn
	mimetypes.mn
)

# Copy source files into build output (optional)
mint_copy_module_sources(${PROJECT_NAME} "codec")

# Install
get_target_property(${PROJECT_NAME}_LIB ${PROJECT_NAME} SOURCES)
install(
	FILES ${${PROJECT_NAME}_LIB}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/codec
)
