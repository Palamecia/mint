project(mint-system)

set(${PROJECT_NAME}_SRC
	date.cpp
	directory.cpp
	errno.cpp
	file.cpp
	filesystem.cpp
	locale.cpp
	mutex.cpp
	os.cpp
	process.cpp
	terminal.cpp
	thread.cpp
	timezone.cpp
)

if (UNIX)
	set(${PROJECT_NAME}_SRC
		${${PROJECT_NAME}_SRC}
		tzfile.cpp
	)
endif()

if (WIN32)
	set(${PROJECT_NAME}_SRC
		${${PROJECT_NAME}_SRC}
		NtProcessInfo.cpp
		winlocale.cpp
		wintz.cpp
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

if (WIN32)
	find_path(ICU_INCLUDE_DIR NAMES icu.h)
	if (ICU_INCLUDE_DIR)
		message(STATUS "International Components for Unicode (ICU) - enabled")
		include_directories(${ICU_INCLUDE_DIR})
		target_link_libraries(${PROJECT_NAME} icu)
		add_definitions(-DMINT_TIMEZONE_WITH_ICU)
	else()
		message(STATUS "International Components for Unicode (ICU) - disabled")
	endif()
endif()

# Install
install_library(${PROJECT_NAME} SUBDIR mint)
