project(module-mint)

add_custom_target(${PROJECT_NAME}
	SOURCES
	assembly.mn
	commandlineparser.mn
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
mint_install_modules(${${PROJECT_NAME}_LIB} SUBDIR mint)
