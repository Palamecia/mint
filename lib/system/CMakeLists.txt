project(module-system)

add_custom_target(
	mint-module-system
SOURCES
	date.mn
	directory.mn
	event.mn
	file-watcher.mn
	file.mn
	filesystem.mn
	locale.mn
	os.mn
	pipe.mn
	poller.mn
	process.mn
	terminal.mn
	thread.mn
	timer.mn
	timezone.mn
	uri.mn
)

# Copy source files into build output (optional)
mint_copy_module_sources(mint-module-system "system")

# Install
get_target_property(${PROJECT_NAME}_SOURCES mint-module-system SOURCES)
install(
	FILES ${${PROJECT_NAME}_SOURCES}
	COMPONENT runtime DESTINATION ${MINT_MODULES_INSTALL_DIR}/system
)
