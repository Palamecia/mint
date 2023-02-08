project(module-minter)

set(${PROJECT_NAME}_LIB
    config.mn
    lsp.mn
    plugin.mn
    version.mn
    workspace.mn
)

add_custom_target(
    ${PROJECT_NAME}
	SOURCES
	${${PROJECT_NAME}_LIB}
)

# Install
install_modules(${${PROJECT_NAME}_LIB} SUBDIR minter)
