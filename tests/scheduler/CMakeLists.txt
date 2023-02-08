project(test-scheduler)

add_executable(
	${PROJECT_NAME}
	destructor.cpp
	process.cpp
	processor.cpp
	scheduler.cpp
)

target_link_libraries(
	${PROJECT_NAME}
	GTest::gtest_main
	GTest::gtest
	mint_library
)

# gtest_add_tests(TARGET ${PROJECT_NAME})
