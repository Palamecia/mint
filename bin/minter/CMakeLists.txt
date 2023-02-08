project(minter)

set(${PROJECT_NAME}_SOURCES
    minter.mn
)

add_custom_target(
    ${PROJECT_NAME}
    SOURCES
	${${PROJECT_NAME}_SOURCES}
)

# Install
install_script(${${PROJECT_NAME}_SOURCES})
