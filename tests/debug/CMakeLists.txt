project(test-debug)

add_executable(test-debug)

target_sources(
	test-debug
PRIVATE
	debug_infos.cpp
)

target_link_libraries(
	test-debug
PRIVATE
	mint::mint
	GTest::gtest
	GTest::gtest_main
)

gtest_discover_tests(test-debug)
