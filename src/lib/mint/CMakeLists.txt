project(mint-mint)

add_library(${PROJECT_NAME}
	MODULE
	assembly.cpp
	evalresultprinter.cpp
	evalresultprinter.h
	function.cpp
	garbagecollector.cpp
	inttypes.cpp
	lang.cpp
	lexicalhandler.cpp
	operator.cpp
	printer.cpp
	string.cpp
	type.cpp
)

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	PREFIX lib
)

target_link_libraries(${PROJECT_NAME} mint_library)

# Install
install_library(${PROJECT_NAME} SUBDIR mint)
