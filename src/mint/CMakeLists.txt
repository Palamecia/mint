project(mint)

# Target mint
add_executable(mint)

target_sources(
	mint
PRIVATE
	main.cpp
)

set_target_properties(
	mint
	PROPERTIES
	OUTPUT_NAME mint
	INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../lib64"
)

target_link_libraries(
	mint
PRIVATE
	libmint
)

# Install
install(
	TARGETS mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_RUNTIME_INSTALL_DIR}
)
