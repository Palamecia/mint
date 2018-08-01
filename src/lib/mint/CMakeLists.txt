project(mint-mint)

set(${PROJECT_NAME}_SRC
	type.cpp
)

add_library(${PROJECT_NAME}
	MODULE
	${${PROJECT_NAME}_SRC}
)

target_link_libraries(${PROJECT_NAME} mint_library)

# Install
install_library(${PROJECT_NAME} SUBDIR mint)
