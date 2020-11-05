project(mintdoc_executable)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Target mint_executable
add_executable(
	${PROJECT_NAME}
	generators/abstractgenerator.cpp
	generators/gollumgenerator.cpp

	definition.cpp
	dictionnary.cpp
	main.cpp
	module.cpp
	parser.cpp
)

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME mintdoc
	INSTALL_RPATH "$ORIGIN/../lib64"
)

target_link_libraries(
	${PROJECT_NAME}
	mint_library
)

# Install
install_executable(${PROJECT_NAME})
