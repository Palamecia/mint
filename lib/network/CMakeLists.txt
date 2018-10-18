project(module-network)

set(${PROJECT_NAME}_ENDPOINTS_LIB
	endpoints/tcpip.mn
)

set(${PROJECT_NAME}_PROTOCOLS_LIB
	protocols/ftp.mn
	protocols/http.mn
)

set(${PROJECT_NAME}_SERIALIZERS_LIB
	serializers/text.mn
)

set(${PROJECT_NAME}_LIB
	channel.mn
	endpoint.mn
	protocol.mn
	scheduler.mn
	serializer.mn
)

add_custom_target(${PROJECT_NAME}
	SOURCES
	${${PROJECT_NAME}_LIB}
	${${PROJECT_NAME}_ENDPOINTS_LIB}
	${${PROJECT_NAME}_PROTOCOLS_LIB}
	${${PROJECT_NAME}_SERIALIZERS_LIB}
)

# Install
install_modules(${${PROJECT_NAME}_LIB} SUBDIR network)
install_modules(${${PROJECT_NAME}_SERIALIZERS_LIB} SUBDIR network/serializers)
