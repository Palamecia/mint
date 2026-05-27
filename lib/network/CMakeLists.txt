project(module-network)

set(${PROJECT_NAME}_ENDPOINTS_SOURCES
	endpoints/ip.mn
	endpoints/sctp-ip.mn
	endpoints/tcp-ip.mn
	endpoints/udp-ip.mn
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
	protocols/jsonrpc/exception.mn
	protocols/jsonrpc/client.mn
	protocols/jsonrpc/message.mn
	protocols/jsonrpc/serializer.mn
	protocols/jsonrpc/server.mn
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

set(${PROJECT_NAME}_SOURCES
	channel.mn
	endpoint.mn
	scheduler.mn
)

add_custom_target(
	mint-module-network
SOURCES
	${${PROJECT_NAME}_SOURCES}
	${${PROJECT_NAME}_ENDPOINTS_SOURCES}
	${${PROJECT_NAME}_PROTOCOLS_SOURCES}
	${${PROJECT_NAME}_SERIALIZERS_SOURCES}
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
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/network/protocols/jsonrpc
)
install(
	FILES ${${PROJECT_NAME}_SERIALIZERS_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/network/serializers
)
