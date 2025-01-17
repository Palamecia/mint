project(test-debug)

add_executable(test-debug)

target_sources(
	test-debug
	PRIVATE
	debuginfos.cpp
)

target_link_libraries(
	test-debug
	GTest::Main
	GTest::GTest
	mint_library
)

gtest_discover_tests(test-debug)
