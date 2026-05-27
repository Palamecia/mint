project(test-system)

add_executable(test-system)

target_sources(
	test-system
PRIVATE
	buffer_stream.cpp
	data_stream.cpp
	error.cpp
	file_stream.cpp
	filesystem.cpp
	plugin.cpp
	terminal.cpp
	utf8.cpp
)

target_link_libraries(
	test-system
PRIVATE
	mint::mint
	GTest::gtest
	GTest::gtest_main
)

gtest_discover_tests(test-system)
