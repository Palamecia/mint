project(module-mint)

add_custom_target(${PROJECT_NAME}
	SOURCES
	assembly.mn
	commandlineparser.mn
	coroutine.mn
	enum.mn
	function.mn
	garbagecollector.mn
	hash.mn
	inttypes.mn
	lang.mn
	lexicalhandler.mn
	lockguard.mn
	math.mn
	operator.mn
	printer.mn
	range.mn
	string.mn
	regex.mn
	type.mn
)

# Install
get_target_property(${PROJECT_NAME}_LIB ${PROJECT_NAME} SOURCES)
install(
	FILES ${${PROJECT_NAME}_LIB}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/mint
)
