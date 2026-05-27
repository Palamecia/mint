project(mint)

# Target mint
add_executable(mint-mint)

set_target_properties(
	mint-mint
	PROPERTIES
	OUTPUT_NAME mint
	INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../lib64"
)

target_sources(
	mint-mint
PRIVATE
	main.cpp
)

target_link_libraries(
	mint-mint
PRIVATE
	mint::mint
)

# Install
install(
	TARGETS mint-mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_RUNTIME_INSTALL_DIR}
)
