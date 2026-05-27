project(module-codec)

add_custom_target(
	mint-module-codec
SOURCES
	iconv.mn
	mime-types.mn
)

# Copy source files into build output (optional)
mint_copy_module_sources(mint-module-codec "codec")

# Install
get_target_property(${PROJECT_NAME}_SOURCES mint-module-codec SOURCES)
install(
	FILES ${${PROJECT_NAME}_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/codec
)
