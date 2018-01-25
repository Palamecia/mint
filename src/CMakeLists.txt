project(mint_executable)

add_subdirectory(lib)
add_subdirectory(libmint)

# Target mint_executable
add_executable(
	${PROJECT_NAME}
	main.cpp
)

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME mint
)

target_link_libraries(
	${PROJECT_NAME}
	mint_library
)

# Install
install_executable(${PROJECT_NAME})
