project(module-container)

add_custom_target(
	mint-module-container
SOURCES
	linked-list.mn
	list.mn
	map.mn
	queue.mn
	red-black-tree.mn
	set.mn
	stack.mn
)

# Copy source files into build output (optional)
mint_copy_module_sources(mint-module-container "container")

# Install
get_target_property(${PROJECT_NAME}_SOURCES mint-module-container SOURCES)
install(
	FILES ${${PROJECT_NAME}_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/container
)
