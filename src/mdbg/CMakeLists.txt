project(mdbg_executable)

# Target mint_executable
add_executable(
	${PROJECT_NAME}
	debugger.cpp
	debugprinter.cpp
	highlighter.cpp
	main.cpp
)

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME mdbg
)

target_link_libraries(
	${PROJECT_NAME}
	mint_library
)

# Install
install_executable(${PROJECT_NAME})
