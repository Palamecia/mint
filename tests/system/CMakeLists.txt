project(test-system)

add_executable(test-system)

target_sources(
	test-system
	PRIVATE
	bufferstream.cpp
	datastream.cpp
	error.cpp
	filestream.cpp
	filesystem.cpp
	plugin.cpp
	terminal.cpp
	utf8.cpp
)

target_link_libraries(
	test-system
	GTest::Main
	GTest::GTest
	mint_library
)

gtest_discover_tests(test-system)
