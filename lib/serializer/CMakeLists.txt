project(module-serializer)

add_custom_target(${PROJECT_NAME}
	SOURCES
	bufferedstream.mn
	datastream.mn
	documentstream.mn
	jsonstream.mn
	stringstream.mn
	textstream.mn
	xmldomstream.mn
	xmlsaxstream.mn
	xmlstream.mn
	yamlstream.mn
)

# Install
get_target_property(${PROJECT_NAME}_LIB ${PROJECT_NAME} SOURCES)
install(
	FILES ${${PROJECT_NAME}_LIB}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/serializer
)
