project(mintdoc)

find_package(Microsoft.GSL CONFIG REQUIRED)

# Target mint
add_executable(mint-mintdoc)

set_target_properties(
	mint-mintdoc
	PROPERTIES
	OUTPUT_NAME mintdoc
	INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../lib64"
)

target_include_directories(
	mint-mintdoc
PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

target_sources(
	mint-mintdoc
PRIVATE
	generators/abstract_generator.cpp
	generators/abstract_generator.h
	generators/gollum_generator.cpp
	generators/gollum_generator.h
	definition.cpp
	definition.h
	dictionary.cpp
	dictionary.h
	doc_lexer.cpp
	doc_lexer.h
	doc_node.cpp
	doc_node.h
	doc_parser.cpp
	doc_parser.h
	main.cpp
	module.cpp
	module.h
	parser.cpp
	parser.h
)

target_link_libraries(
	mint-mintdoc
PRIVATE
	mint::mint
	Microsoft.GSL::GSL
)

# Install
install(
	TARGETS mint-mintdoc
	RUNTIME COMPONENT runtime DESTINATION ${MINT_RUNTIME_INSTALL_DIR}
)
