project(mint-test)

add_library(mint-test MODULE)

target_sources(
	mint-test
PRIVATE
	case.cpp
	errorcatcher.cpp
	suite.cpp
)

set_target_properties(
	mint-test
	PROPERTIES
	PREFIX lib
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_link_libraries(
	mint-test
PRIVATE
	libmint
)

# Install
install(
	TARGETS mint-test
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)
