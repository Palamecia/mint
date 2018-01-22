project(mint_executable)

add_subdirectory(lib)
add_subdirectory(libmint)

# Target mint
add_executable(
	${PROJECT_NAME}
	main.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME mint)

target_link_libraries(${PROJECT_NAME} mint_library)

# Install
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${MINT_INSTALL_RUNTIME_DIR})
