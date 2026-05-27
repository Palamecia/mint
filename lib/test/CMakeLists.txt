project(module-test)

add_custom_target(
	mint-module-test
SOURCES
	case.mn
	error-catcher.mn
	suite.mn
)

# Copy source files into build output (optional)
mint_copy_module_sources(mint-module-test "test")

# Install
get_target_property(${PROJECT_NAME}_SOURCES mint-module-test SOURCES)
install(
	FILES ${${PROJECT_NAME}_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/test
)
