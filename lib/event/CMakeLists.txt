project(module-event)

add_custom_target(
	mint-module-event
SOURCES
	event.mn
	file.mn
	pipe.mn
	stream-synchronizer.mn
	timer.mn
	watcher.mn
)

# Copy source files into build output (optional)
mint_copy_module_sources(mint-module-event "event")

# Install
get_target_property(${PROJECT_NAME}_SOURCES mint-module-event SOURCES)
install(
	FILES ${${PROJECT_NAME}_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/event
)
