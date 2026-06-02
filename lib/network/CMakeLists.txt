project(module-network)

set(${PROJECT_NAME}_ENDPOINTS_SOURCES
	endpoints/ip.mn
	endpoints/local.mn
	endpoints/stdio.mn
)

set(${PROJECT_NAME}_FTP_SOURCES
	protocols/ftp/client.mn
	protocols/ftp/message.mn
	protocols/ftp/serializer.mn
	protocols/ftp/server.mn
)

set(${PROJECT_NAME}_HTTP_SOURCES
	protocols/http/client.mn
	protocols/http/message.mn
	protocols/http/serializer.mn
	protocols/http/server.mn
)

set(${PROJECT_NAME}_JSONRPC_SOURCES
	protocols/json-rpc/exception.mn
	protocols/json-rpc/client.mn
	protocols/json-rpc/message.mn
	protocols/json-rpc/serializer.mn
	protocols/json-rpc/server.mn
)

set(${PROJECT_NAME}_PROTOCOLS_SOURCES
	${${PROJECT_NAME}_FTP_SOURCES}
	${${PROJECT_NAME}_HTTP_SOURCES}
	${${PROJECT_NAME}_JSONRPC_SOURCES}
)

set(${PROJECT_NAME}_SERIALIZERS_SOURCES
	serializers/data.mn
	serializers/datagram.mn
	serializers/server.mn
	serializers/text.mn
)

set(${PROJECT_NAME}_SOCKETS_SOURCES
	sockets/ip.mn
	sockets/local.mn
	sockets/sctp.mn
	sockets/stdio.mn
	sockets/tcp.mn
	sockets/udp.mn
)

set(${PROJECT_NAME}_SOURCES
	channel.mn
	endpoint.mn
	poller.mn
	socket.mn
)

add_custom_target(
	mint-module-network
SOURCES
	${${PROJECT_NAME}_SOURCES}
	${${PROJECT_NAME}_ENDPOINTS_SOURCES}
	${${PROJECT_NAME}_PROTOCOLS_SOURCES}
	${${PROJECT_NAME}_SERIALIZERS_SOURCES}
	${${PROJECT_NAME}_SOCKETS_SOURCES}
)

# Copy source files into build output (optional)
mint_copy_module_sources(mint-module-network "network")

# Install
install(
	FILES ${${PROJECT_NAME}_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/network
)
install(
	FILES ${${PROJECT_NAME}_ENDPOINTS_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/network/endpoints
)
install(
	FILES ${${PROJECT_NAME}_FTP_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/network/protocols/ftp
)
install(
	FILES ${${PROJECT_NAME}_HTTP_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/network/protocols/http
)
install(
	FILES ${${PROJECT_NAME}_JSONRPC_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/network/protocols/json-rpc
)
install(
	FILES ${${PROJECT_NAME}_SERIALIZERS_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/network/serializers
)
install(
	FILES ${${PROJECT_NAME}_SOCKETS_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/network/sockets
)
