project(module-serializer)

set(${PROJECT_NAME}_LIB
	bufferedstream.mn
	datastream.mn
	jsonstream.mn
	stringstream.mn
	textstream.mn
	xmlstream.mn
)

add_custom_target(${PROJECT_NAME}
	SOURCES
	${${PROJECT_NAME}_LIB}
)

# Install
install_modules(${${PROJECT_NAME}_LIB} SUBDIR serializer)
