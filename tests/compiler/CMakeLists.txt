project(test-compiler)

add_executable(test-compiler)

target_sources(
	test-compiler
	PRIVATE
	buildtool.cpp
	compiler.cpp
	lexer.cpp
)

target_link_libraries(
	test-compiler
	GTest::Main
	GTest::GTest
	mint_library
)

gtest_discover_tests(test-compiler)
