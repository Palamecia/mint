project(mint-network)

set(MODULE_NAME network)

add_library(mint-network MODULE)

target_SOURCES(
	mint-network
PRIVATE
	ip.cpp
	scheduler.cpp
	scheduler.h
	socket.cpp
	socket.h
	tcpip.cpp
	udpip.cpp
)

set_target_properties(
	mint-network
	PROPERTIES
	PREFIX lib
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_link_libraries(
	mint-network
PRIVATE
	libmint
)

if (WIN32)
    target_link_libraries(
		mint-network
	PRIVATE
		ws2_32
	)
endif()

# Install
install(
	TARGETS mint-network
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)

add_dependencies(mint-network module-network)
