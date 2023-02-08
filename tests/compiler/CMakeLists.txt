project(test-compiler)

add_executable(
	${PROJECT_NAME}
	buildtool.cpp
	compiler.cpp
	lexer.cpp
)

target_link_libraries(
	${PROJECT_NAME}
	GTest::gtest_main
	GTest::gtest
	mint_library
)

# gtest_add_tests(TARGET ${PROJECT_NAME})
