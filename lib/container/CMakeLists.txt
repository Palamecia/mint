project(module-container)

add_custom_target(${PROJECT_NAME}
	SOURCES
	linkedlist.mn
	list.mn
	map.mn
	queue.mn
	redblacktree.mn
	set.mn
	stack.mn
)

# Copy source files into build output (optional)
mint_copy_module_sources(${PROJECT_NAME} "container")

# Install
get_target_property(${PROJECT_NAME}_LIB ${PROJECT_NAME} SOURCES)
install(
	FILES ${${PROJECT_NAME}_LIB}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/container
)
