project(mint_library)

add_definitions(-DBUILD_MINT_LIB)

add_subdirectory(ast)
add_subdirectory(compiler)
add_subdirectory(debug)
add_subdirectory(memory)
add_subdirectory(scheduler)
add_subdirectory(system)

# Target mint_library
add_library(
	${PROJECT_NAME}
	SHARED
	$<TARGET_OBJECTS:ast>
	$<TARGET_OBJECTS:compiler>
	$<TARGET_OBJECTS:debug>
	$<TARGET_OBJECTS:memory>
	$<TARGET_OBJECTS:scheduler>
	$<TARGET_OBJECTS:system>
)

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME mint
	PREFIX lib
)

if (UNIX)
	target_link_libraries(
		${PROJECT_NAME}
		pthread
		dl
	)
elseif (WIN32)
	target_link_libraries(
		${PROJECT_NAME}
		userenv
	)
endif()

add_dependencies(${PROJECT_NAME} scheduler)

# Install
install_library(${PROJECT_NAME})
