project(lib)

add_subdirectory(codec)
add_subdirectory(container)
add_subdirectory(exception)
add_subdirectory(event)
add_subdirectory(mint)
add_subdirectory(network)
add_subdirectory(serializer)
add_subdirectory(system)
add_subdirectory(test)

set(module_LIB
	math.mn
)

add_custom_target(module
	SOURCES
	${module_LIB}
)

# Install
install_modules(${module_LIB})
