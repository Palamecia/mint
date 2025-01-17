project(test-scheduler)

add_executable(test-scheduler)

target_sources(
	test-scheduler
	PRIVATE
	destructor.cpp
	inputstream.cpp
	output.cpp
	process.cpp
	processor.cpp
	scheduler.cpp
)

target_link_libraries(
	test-scheduler
	GTest::Main
	GTest::GTest
	mint_library
)

gtest_discover_tests(test-scheduler)
