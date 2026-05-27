project(test-compiler)

add_executable(test-compiler)

target_sources(
	test-compiler
PRIVATE
	build_tools.cpp
	compiler.cpp
	lexer.cpp
	lexical_handler.cpp
)

target_link_libraries(
	test-compiler
PRIVATE
	mint::mint
	GTest::gtest
	GTest::gtest_main
)

gtest_discover_tests(test-compiler)
