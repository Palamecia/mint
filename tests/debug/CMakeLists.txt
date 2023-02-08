project(test-debug)

add_executable(
	${PROJECT_NAME}
	debuginfos.cpp
)

target_link_libraries(
	${PROJECT_NAME}
	GTest::gtest_main
	GTest::gtest
	mint_library
)

# gtest_add_tests(TARGET ${PROJECT_NAME})
