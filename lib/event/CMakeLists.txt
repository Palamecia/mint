project(module-event)

set(${PROJECT_NAME}_LIB
	event.mn
	file.mn
	pipe.mn
	streamsynchronizer.mn
	timer.mn
	watcher.mn
)

add_custom_target(${PROJECT_NAME}
	SOURCES
	${${PROJECT_NAME}_LIB}
)

# Install
install_modules(${${PROJECT_NAME}_LIB} SUBDIR event)
