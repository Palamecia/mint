project(test-compiler)

set(${PROJECT_NAME}_SRC
	buildtool.cpp
	compiler.cpp
	lexer.cpp
)

add_executable(
	${PROJECT_NAME}
	${${PROJECT_NAME}_SRC}
)

target_link_libraries(
	${PROJECT_NAME}
	GTest::gtest_main
	GTest::gtest
	mint_library
)

# gtest_add_tests(TARGET ${PROJECT_NAME})
