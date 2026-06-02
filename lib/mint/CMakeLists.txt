project(module-mint)

add_custom_target(
	mint-module-mint
SOURCES
	assembly.mn
	commandline-parser.mn
	coroutine.mn
	enum.mn
	function.mn
	garbage-collector.mn
	hash.mn
	int-types.mn
	lang.mn
	lexical-handler.mn
	lock-guard.mn
	math.mn
	operator.mn
	printer.mn
	range.mn
	stacktrace.mn
	string.mn
	regex.mn
	type.mn
)

# Copy source files into build output (optional)
mint_copy_module_sources(mint-module-mint "mint")

# Install
get_target_property(${PROJECT_NAME}_SOURCES mint-module-mint SOURCES)
install(
	FILES ${${PROJECT_NAME}_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/mint
)
