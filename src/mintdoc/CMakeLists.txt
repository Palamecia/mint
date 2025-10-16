project(mintdoc)

find_package(Microsoft.GSL CONFIG REQUIRED)

# Target mint
add_executable(mintdoc)

target_include_directories(
	mintdoc
PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

target_sources(
	mintdoc
PRIVATE
	generators/abstractgenerator.cpp
	generators/abstractgenerator.h
	generators/gollumgenerator.cpp
	generators/gollumgenerator.h
	definition.cpp
	definition.h
	dictionary.cpp
	dictionary.h
	doclexer.cpp
	doclexer.h
	docnode.cpp
	docnode.h
	docparser.cpp
	docparser.h
	main.cpp
	module.cpp
	module.h
	parser.cpp
	parser.h
)

set_target_properties(
	mintdoc
	PROPERTIES
	OUTPUT_NAME mintdoc
	INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../lib64"
)

target_link_libraries(
	mintdoc
PRIVATE
	libmint
	Microsoft.GSL::GSL
)

# Install
install(
	TARGETS mintdoc
	RUNTIME COMPONENT runtime DESTINATION ${MINT_RUNTIME_INSTALL_DIR}
)
