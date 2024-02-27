project(mintdoc_executable)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

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
	dictionnary.h
	module.h
	parser.h
)

set(${PROJECT_NAME}_SOURCES
	definition.cpp
	dictionnary.cpp
	module.cpp
	parser.cpp
)

# Target mint_executable
add_executable(
	${PROJECT_NAME}
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
