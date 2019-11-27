project(module-container)

set(${PROJECT_NAME}_LIB
	linkedlist.mn
	list.mn
	map.mn
	queue.mn
	redblacktree.mn
	set.mn
	stack.mn
)

add_custom_target(${PROJECT_NAME}
	SOURCES
	${${PROJECT_NAME}_LIB}
)

# Install
install_modules(${${PROJECT_NAME}_LIB} SUBDIR container)
