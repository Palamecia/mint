project(module-runtime)

add_custom_target(
	mint-module-runtime
SOURCES
	operation.mn
	promise.mn
	scheduler.mn
	stream-synchronizer.mn
)

# Copy source files into build output (optional)
mint_copy_module_sources(mint-module-runtime "runtime")

# Install
get_target_property(${PROJECT_NAME}_SOURCES mint-module-runtime SOURCES)
install(
	FILES ${${PROJECT_NAME}_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/runtime
)
