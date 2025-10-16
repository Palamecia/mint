project(test-compiler)

add_executable(test-compiler)

target_sources(
	test-compiler
PRIVATE
	buildtool.cpp
	compiler.cpp
	lexer.cpp
	lexicalhandler.cpp
)

target_link_libraries(
	test-compiler
PRIVATE
	libmint
	GTest::gtest
	GTest::gtest_main
)

gtest_discover_tests(test-compiler)
