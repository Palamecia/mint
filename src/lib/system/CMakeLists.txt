project(mint-system)

set(${PROJECT_NAME}_SRC
	date.cpp
	directory.cpp
	file.cpp
	locale.cpp
	mutex.cpp
	process.cpp
	terminal.cpp
	thread.cpp
	timezone.cpp
)

if (WIN32)
	set(${PROJECT_NAME}_SRC
		${${PROJECT_NAME}_SRC}
		NtProcessInfo.cpp
	)
endif()

add_library(${PROJECT_NAME}
	MODULE
	${${PROJECT_NAME}_SRC}
)

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	PREFIX lib
)

target_link_libraries(${PROJECT_NAME} mint_library)

# Install
install_library(${PROJECT_NAME} SUBDIR mint)
