project(libmint)

add_subdirectory(ast)
add_subdirectory(compiler)
add_subdirectory(memory)
add_subdirectory(scheduler)
add_subdirectory(system)

# Target libmint
add_library(${PROJECT_NAME}
	SHARED
	$<TARGET_OBJECTS:ast>
	$<TARGET_OBJECTS:compiler>
	$<TARGET_OBJECTS:memory>
	$<TARGET_OBJECTS:scheduler>
	$<TARGET_OBJECTS:system>
)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME mint)

if (UNIX)
	target_link_libraries(${PROJECT_NAME} dl readline)
endif()

add_dependencies(${PROJECT_NAME} scheduler)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${MINT_INSTALL_LIBRARY_DIR})