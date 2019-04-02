project(module-event)

set(${PROJECT_NAME}_LIB
	event.mn
	pipe.mn
	timer.mn
	watcher.mn
)

add_custom_target(${PROJECT_NAME}
	SOURCES
	${${PROJECT_NAME}_LIB}
)

# Install
install_modules(${${PROJECT_NAME}_LIB} SUBDIR event)
