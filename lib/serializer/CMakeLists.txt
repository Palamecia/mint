project(module-serializer)

add_custom_target(
	mint-module-serializer
SOURCES
	buffered-stream.mn
	data-stream.mn
	document-stream.mn
	json-stream.mn
	string-stream.mn
	text-stream.mn
	xml-dom-stream.mn
	xml-sax-stream.mn
	xml-stream.mn
	yaml-stream.mn
)

# Copy source files into build output (optional)
mint_copy_module_sources(mint-module-serializer "serializer")

# Install
get_target_property(${PROJECT_NAME}_SOURCES mint-module-serializer SOURCES)
install(
	FILES ${${PROJECT_NAME}_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/serializer
)
