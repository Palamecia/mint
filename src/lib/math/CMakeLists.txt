project(mint-math)

set(${PROJECT_NAME}_SRC
	math.cpp
)

add_library(${PROJECT_NAME}
	MODULE
	${${PROJECT_NAME}_SRC}
)

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	PREFIX lib
)

target_link_libraries(${PROJECT_NAME} mint_library)

# Install
install_library(${PROJECT_NAME} SUBDIR mint)
