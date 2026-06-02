project(module-exception)

add_custom_target(
	mint-module-exception
SOURCES
	codec.mn
	container.mn
	exception.mn
	network.mn
	system.mn
)

# Copy source files into build output (optional)
mint_copy_module_sources(mint-module-exception "exception")

# Install
get_target_property(${PROJECT_NAME}_SOURCES mint-module-exception SOURCES)
install(
	FILES ${${PROJECT_NAME}_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/exception
)
