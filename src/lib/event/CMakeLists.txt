project(mint-event)

set(${PROJECT_NAME}_SOURCES
	event.cpp
	file.cpp
	pipe.cpp
	timer.cpp
	watcher.cpp
)

add_library(${PROJECT_NAME}
	MODULE
	${${PROJECT_NAME}_SOURCES}
)

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	PREFIX lib
)

target_link_libraries(${PROJECT_NAME} mint_library)

# Install
install_library(${PROJECT_NAME} SUBDIR mint)
