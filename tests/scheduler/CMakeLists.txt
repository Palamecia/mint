project(test-scheduler)

add_executable(test-scheduler)

target_sources(
	test-scheduler
PRIVATE
	destructor.cpp
	input_stream.cpp
	output.cpp
	process.cpp
	processor.cpp
	scheduler.cpp
)

target_link_libraries(
	test-scheduler
PRIVATE
	mint::mint
	GTest::gtest
	GTest::gtest_main
)

gtest_discover_tests(test-scheduler)
