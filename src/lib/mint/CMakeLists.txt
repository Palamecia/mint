project(mint-mint)

add_library(mint-mint MODULE)

target_sources(
	mint-mint
PRIVATE
	assembly.cpp
	evalresultprinter.cpp
	evalresultprinter.h
	function.cpp
	garbagecollector.cpp
	inttypes.cpp
	lang.cpp
	lexicalhandler.cpp
	math.cpp
	operator.cpp
	printer.cpp
	string.cpp
	type.cpp
)

set_target_properties(
	mint-mint
	PROPERTIES
	PREFIX lib
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_link_libraries(
	mint-mint
PRIVATE
	libmint
)

# Install
install(
	TARGETS mint-mint
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)
