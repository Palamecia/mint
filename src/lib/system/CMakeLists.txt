project(mint-system)

set(${PROJECT_NAME}_SOURCES
	date.cpp
	directory.cpp
	errno.cpp
	file.cpp
	filesystem.cpp
	future.cpp
	locale.cpp
	mutex.cpp
	os.cpp
	process.cpp
	terminal.cpp
	thread.cpp
	timezone.cpp
)

if (UNIX)
	set(${PROJECT_NAME}_SOURCES
		${${PROJECT_NAME}_SOURCES}
		unix/tzfile.cpp
	)
endif()

if (WIN32)
	set(${PROJECT_NAME}_SOURCES
		${${PROJECT_NAME}_SOURCES}
		win32/NtProcessInfo.cpp
		win32/winlocale.cpp
		win32/wintz.cpp
	)
endif()

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

if (WIN32)
	if (ICU_LIBRARIES)
		target_link_libraries(${PROJECT_NAME} ${ICU_LIBRARIES})
	endif()
endif()

# Install
install_library(${PROJECT_NAME} SUBDIR mint)
