project(mint-mint)

set(${PROJECT_NAME}_SRC
	assembly.cpp
	function.cpp
	garbagecollector.cpp
	inttypes.cpp
	lang.cpp
	operator.cpp
	type.cpp
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
