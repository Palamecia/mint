project(test-debug)

add_executable(test-debug)

target_sources(
	test-debug
PRIVATE
	debuginfos.cpp
)

target_link_libraries(
	test-debug
PRIVATE
	libmint
	GTest::gtest
	GTest::gtest_main
)

gtest_discover_tests(test-debug)
