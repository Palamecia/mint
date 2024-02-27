project(module-test)

add_custom_target(${PROJECT_NAME}
	SOURCES
	case.mn
	suite.mn
)

# Install
get_target_property(${PROJECT_NAME}_LIB ${PROJECT_NAME} SOURCES)
mint_install_modules(${${PROJECT_NAME}_LIB} SUBDIR test)
