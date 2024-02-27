project(mint_executable)

# Target mint_executable
add_executable(${PROJECT_NAME})

target_sources(
	${PROJECT_NAME}
	PRIVATE
	main.cpp
)

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME mint
	INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../lib64"
)

target_link_libraries(
	${PROJECT_NAME}
	mint_library
)

# Install
mint_install_executable(${PROJECT_NAME})
