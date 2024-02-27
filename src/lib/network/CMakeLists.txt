project(mint-network)

set(MODULE_NAME network)

set(${PROJECT_NAME}_SOURCES
	ip.cpp
	scheduler.cpp
	scheduler.h
	socket.cpp
	socket.h
	tcpip.cpp
	udpip.cpp
)

add_library(${PROJECT_NAME}
	MODULE
	${${PROJECT_NAME}_SOURCES}
)

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	PREFIX lib
)

target_link_libraries(${PROJECT_NAME} mint_library)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)
endif()

# Install
mint_install_library(${PROJECT_NAME} SUBDIR mint)
