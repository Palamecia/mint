project(mint-event)

add_library(mint-lib-module-event MODULE)

set_target_properties(
	mint-lib-module-event
	PROPERTIES
	PREFIX lib
	OUTPUT_NAME mint-event
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_SOURCES(
	mint-lib-module-event
PRIVATE
	event.cpp
	file.cpp
	pipe.cpp
	timer.cpp
	watcher.cpp
)

target_link_libraries(
	mint-lib-module-event
PRIVATE
	mint::mint
)

# Install
install(
	TARGETS mint-lib-module-event
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)

add_dependencies(mint-lib-module-event mint-module-event)
