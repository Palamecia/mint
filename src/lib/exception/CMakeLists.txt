project(mint-exception)

add_library(mint-lib-module-exception MODULE)

set_target_properties(
	mint-lib-module-exception
	PROPERTIES
	PREFIX lib
	OUTPUT_NAME mint-exception
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_sources(
	mint-lib-module-exception
PRIVATE
	exception.cpp
)

target_link_libraries(
	mint-lib-module-exception
PRIVATE
	mint::mint
)

# Install
install(
	TARGETS mint-lib-module-exception
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)

add_dependencies(mint-lib-module-exception mint-module-exception)
