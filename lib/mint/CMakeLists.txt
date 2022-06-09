project(module-mint)

set(${PROJECT_NAME}_LIB
	assembly.mn
	commandlineparser.mn
	enum.mn
	function.mn
	garbagecollector.mn
	hash.mn
	inttypes.mn
	lockguard.mn
	lang.mn
	operator.mn
	range.mn
	string.mn
	regex.mn
	type.mn
)

add_custom_target(${PROJECT_NAME}
	SOURCES
	${${PROJECT_NAME}_LIB}
)

# Install
install_modules(${${PROJECT_NAME}_LIB} SUBDIR mint)
