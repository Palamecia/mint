project(mint-runtime)

add_library(mint-lib-module-runtime MODULE)

set_target_properties(
	mint-lib-module-runtime
	PROPERTIES
	PREFIX lib
	OUTPUT_NAME mint-runtime
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_sources(
	mint-lib-module-runtime
PRIVATE
	operation.cpp
	scheduler.cpp
)

target_link_libraries(
	mint-lib-module-runtime
PRIVATE
	mint::mint
)

# Install
install(
	TARGETS mint-lib-module-runtime
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)

add_dependencies(mint-lib-module-runtime mint-module-runtime)
