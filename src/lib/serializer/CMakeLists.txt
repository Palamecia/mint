project(mint-serializer)

add_library(mint-lib-module-serializer MODULE)

set_target_properties(
	mint-lib-module-serializer
	PROPERTIES
	PREFIX lib
	OUTPUT_NAME mint-serializer
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_sources(
	mint-lib-module-serializer
PRIVATE
	data_stream.cpp
)

target_link_libraries(
	mint-lib-module-serializer
PRIVATE
	mint::mint
)

# Install
install(
	TARGETS mint-lib-module-serializer
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)

add_dependencies(mint-lib-module-serializer mint-module-serializer)
