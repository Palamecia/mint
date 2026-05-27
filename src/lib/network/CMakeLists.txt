project(mint-network)

add_library(mint-lib-module-network MODULE)

set_target_properties(
	mint-lib-module-network
	PROPERTIES
	PREFIX lib
	OUTPUT_NAME mint-network
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_SOURCES(
	mint-lib-module-network
PRIVATE
	ip.cpp
	scheduler.cpp
	scheduler.h
	socket.cpp
	socket.h
	tcp_ip.cpp
	udp_ip.cpp
)

target_link_libraries(
	mint-lib-module-network
PRIVATE
	mint::mint
)

if (WIN32)
    target_link_libraries(
		mint-lib-module-network
	PRIVATE
		ws2_32
	)
endif()

# Install
install(
	TARGETS mint-lib-module-network
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)

add_dependencies(mint-lib-module-network mint-module-network)
