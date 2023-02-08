project(module-network)

set(${PROJECT_NAME}_ENDPOINTS_LIB
	endpoints/ip.mn
	endpoints/sctpip.mn
	endpoints/tcpip.mn
	endpoints/textstream.mn
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

set(${PROJECT_NAME}_JSONRPC_LIB
	protocols/jsonrpc/exception.mn
	protocols/jsonrpc/client.mn
	protocols/jsonrpc/message.mn
	protocols/jsonrpc/serializer.mn
	protocols/jsonrpc/server.mn
)

set(${PROJECT_NAME}_PROTOCOLS_LIB
	${${PROJECT_NAME}_FTP_LIB}
	${${PROJECT_NAME}_HTTP_LIB}
	${${PROJECT_NAME}_JSONRPC_LIB}
)

set(${PROJECT_NAME}_SERIALIZERS_LIB
	serializers/data.mn
	serializers/datagram.mn
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
install_modules(${${PROJECT_NAME}_JSONRPC_LIB} SUBDIR network/protocols/jsonrpc)
install_modules(${${PROJECT_NAME}_SERIALIZERS_LIB} SUBDIR network/serializers)
