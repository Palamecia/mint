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

# Install
get_target_property(${PROJECT_NAME}_LIB ${PROJECT_NAME} SOURCES)
mint_install_modules(${${PROJECT_NAME}_LIB} SUBDIR container)
