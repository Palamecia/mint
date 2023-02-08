project(mint-serializer)

set(${PROJECT_NAME}_SOURCES
	datastream.cpp
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

# Install
install_library(${PROJECT_NAME} SUBDIR mint)
