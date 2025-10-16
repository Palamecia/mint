project(module-codec)

add_custom_target(${PROJECT_NAME}
	SOURCES
	iconv.mn
	mimetypes.mn
)

# Install
get_target_property(${PROJECT_NAME}_LIB ${PROJECT_NAME} SOURCES)
install(
	FILES ${${PROJECT_NAME}_LIB}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/codec
)
