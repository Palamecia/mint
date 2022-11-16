project(module-network)

set(${PROJECT_NAME}_ENDPOINTS_LIB
	endpoints/ip.mn
	endpoints/tcpip.mn
	endpoints/udpip.mn
)

set(${PROJECT_NAME}_FTP_LIB
	protocols/ftp/client.mn
	protocols/ftp/message.mn
	protocols/ftp/serializer.mn
	protocols/ftp/server.mn
)

set(${PROJECT_NAME}_HTTP_LIB
	protocols/http/client.mn
	protocols/http/message.mn
	protocols/http/serializer.mn
	protocols/http/server.mn
)

set(${PROJECT_NAME}_SERIALIZERS_LIB
	serializers/data.mn
	serializers/server.mn
	serializers/text.mn
)

set(${PROJECT_NAME}_LIB
	channel.mn
	endpoint.mn
	scheduler.mn
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
install_modules(${${PROJECT_NAME}_ENDPOINTS_LIB} SUBDIR network/endpoints)
install_modules(${${PROJECT_NAME}_FTP_LIB} SUBDIR network/protocols/ftp)
install_modules(${${PROJECT_NAME}_HTTP_LIB} SUBDIR network/protocols/http)
install_modules(${${PROJECT_NAME}_SERIALIZERS_LIB} SUBDIR network/serializers)
