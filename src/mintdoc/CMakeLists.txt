project(mintdoc_executable)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

# Target mint_executable
add_executable(${PROJECT_NAME})

set(${PROJECT_NAME}_GENERATORS_HEADERS
	generators/abstractgenerator.h
	generators/gollumgenerator.h
)

set(${PROJECT_NAME}_GENERATORS_SOURCES
	generators/abstractgenerator.cpp
	generators/gollumgenerator.cpp
)

set(${PROJECT_NAME}_HEADERS
	definition.h
	dictionary.h
	doclexer.h
	docnode.h
	docparser.h
	module.h
	parser.h
)

set(${PROJECT_NAME}_SOURCES
	definition.cpp
	dictionary.cpp
	doclexer.cpp
	docnode.cpp
	docparser.cpp
	module.cpp
	parser.cpp
)

target_sources(
	${PROJECT_NAME}
	PRIVATE
	${${PROJECT_NAME}_GENERATORS_HEADERS}
	${${PROJECT_NAME}_GENERATORS_SOURCES}
	${${PROJECT_NAME}_HEADERS}
	${${PROJECT_NAME}_SOURCES}
	main.cpp
)

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME mintdoc
	INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../lib64"
)

target_link_libraries(
	${PROJECT_NAME}
	mint_library
)

# Install
mint_install_executable(${PROJECT_NAME})
