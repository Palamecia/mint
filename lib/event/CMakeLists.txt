project(module-event)

add_custom_target(${PROJECT_NAME}
	SOURCES
	event.mn
	file.mn
	pipe.mn
	streamsynchronizer.mn
	timer.mn
	watcher.mn
)

# Install
get_target_property(${PROJECT_NAME}_LIB ${PROJECT_NAME} SOURCES)
mint_install_modules(${${PROJECT_NAME}_LIB} SUBDIR event)
