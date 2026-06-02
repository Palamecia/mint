project(mint-system)

find_package(ICU REQUIRED COMPONENTS uc i18n data)

add_library(mint-lib-module-system MODULE)

set_target_properties(
	mint-lib-module-system
	PROPERTIES
	PREFIX lib
	OUTPUT_NAME mint-system
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_sources(
	mint-lib-module-system
PRIVATE
	date.cpp
	directory.cpp
	errno.cpp
	event.cpp
	file_watcher.cpp
	file.cpp
	filesystem.cpp
	locale.cpp
	mutex.cpp
	os.cpp
	pipe.cpp
	poll.cpp
	process.cpp
	promise.cpp
	terminal.cpp
	thread.cpp
	timer.cpp
	timezone.cpp
)

if (UNIX)
	target_link_libraries(
		mint-lib-module-system
	PUBLIC
		liburing::io_uring
	)
elseif (WIN32)
	target_sources(
		mint-lib-module-system
	PRIVATE
		win32/NtProcessInfo.cpp
		win32/NtProcessInfo.h
		win32/winlocale.cpp
		win32/winlocale.h
	)
endif()

target_link_libraries(
	mint-lib-module-system
PRIVATE
	mint::mint
	ICU::i18n
	ICU::uc
	ICU::data
)

# Install
install(
	TARGETS mint-lib-module-system
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)

add_dependencies(mint-lib-module-system mint-module-system)
