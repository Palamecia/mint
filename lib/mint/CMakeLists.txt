project(module-mint)

set(${PROJECT_NAME}_LIB
	garbagecollector.mn
	inttypes.mn
	lockguard.mn
	lang.mn
	type.mn
)

add_custom_target(${PROJECT_NAME}
	SOURCES
	${${PROJECT_NAME}_LIB}
)

# Install
install_modules(${${PROJECT_NAME}_LIB} SUBDIR mint)
