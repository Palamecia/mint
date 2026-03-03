project(mint-serializer)

add_library(mint-serializer MODULE)

target_sources(
	mint-serializer
PRIVATE
	datastream.cpp
)

set_target_properties(
	mint-serializer
	PROPERTIES
	PREFIX lib
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_link_libraries(
	mint-serializer
PRIVATE
	libmint
)

# Install
install(
	TARGETS mint-serializer
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)

add_dependencies(mint-serializer module-serializer)
