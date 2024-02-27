project(mint-test)

set(${PROJECT_NAME}_SOURCES
	case.cpp
	suite.cpp
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
mint_install_library(${PROJECT_NAME} SUBDIR mint)
