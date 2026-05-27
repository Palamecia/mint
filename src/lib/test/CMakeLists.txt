project(mint-test)

add_library(mint-lib-module-test MODULE)

target_sources(
	mint-lib-module-test
PRIVATE
	case.cpp
	error_catcher.cpp
	suite.cpp
)

set_target_properties(
	mint-lib-module-test
	PROPERTIES
	PREFIX lib
	OUTPUT_NAME mint-test
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_link_libraries(
	mint-lib-module-test
PRIVATE
	mint::mint
)

# Install
install(
	TARGETS mint-lib-module-test
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)

add_dependencies(mint-lib-module-test mint-module-test)
