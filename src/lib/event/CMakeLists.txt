project(mint-event)

add_library(mint-event MODULE)

target_SOURCES(
	mint-event
PRIVATE
	event.cpp
	file.cpp
	pipe.cpp
	timer.cpp
	watcher.cpp
)

set_target_properties(
	mint-event
	PROPERTIES
	PREFIX lib
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_link_libraries(
	mint-event
PRIVATE
	libmint
)

# Install
install(
	TARGETS mint-event
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)

add_dependencies(mint-event module-event)
