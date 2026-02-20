project(mint-system)

find_package(ICU REQUIRED COMPONENTS uc i18n data)

add_library(mint-system MODULE)

target_sources(
	mint-system
PRIVATE
	date.cpp
	directory.cpp
	errno.cpp
	file.cpp
	filesystem.cpp
	locale.cpp
	mutex.cpp
	os.cpp
	process.cpp
	promise.cpp
	terminal.cpp
	thread.cpp
	timezone.cpp
)

if (WIN32)
	target_sources(
		mint-system
	PRIVATE
		win32/NtProcessInfo.cpp
		win32/NtProcessInfo.h
		win32/winlocale.cpp
		win32/winlocale.h
	)
endif()

set_target_properties(
	mint-system
	PROPERTIES
	PREFIX lib
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_link_libraries(
	mint-system
PRIVATE
	libmint
	ICU::i18n
	ICU::uc
	ICU::data
)

# Install
install(
	TARGETS mint-system
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)
