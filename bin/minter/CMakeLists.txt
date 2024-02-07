project(minter)

set(${PROJECT_NAME}_LIBRARIES
	minter/config.mn
	minter/lsp.mn
	minter/plugin.mn
	minter/version.mn
	minter/workspace.mn
)

set(${PROJECT_NAME}_PLUGINS
	plugins/default.mn
)

set(${PROJECT_NAME}_SOURCES
    minter.mn
)

source_group("Libraries" FILES ${${PROJECT_NAME}_LIBRARIES})
source_group("Plugins" FILES ${${PROJECT_NAME}_PLUGINS})

add_custom_target(
    ${PROJECT_NAME}
    SOURCES
	${${PROJECT_NAME}_LIBRARIES}
	${${PROJECT_NAME}_PLUGINS}
	${${PROJECT_NAME}_SOURCES}
)

# Install
install_scripts(${${PROJECT_NAME}_LIBRARIES} SUBDIR minter/minter)
install_scripts(${${PROJECT_NAME}_PLUGINS} SUBDIR minter/plugins)
install_scripts(${${PROJECT_NAME}_SOURCES} SUBDIR minter)
