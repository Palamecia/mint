project(mint-network)

set(MODULE_NAME network)

set(${PROJECT_NAME}_SRC
	scheduler.cpp
	scheduler.h
	tcpip.cpp
	udpip.cpp
)

add_library(${PROJECT_NAME}
	MODULE
	${${PROJECT_NAME}_SRC}
)

target_link_libraries(${PROJECT_NAME} mint_library)

# Install
install_library(${PROJECT_NAME} SUBDIR mint)
