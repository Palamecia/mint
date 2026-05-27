project(test-ast)

add_executable(test-ast)

target_sources(
	test-ast
PRIVATE
	abstract_syntax_tree.cpp
	cursor.cpp
	file_printer.cpp
	module.cpp
	node.cpp
	printer.cpp
)

target_link_libraries(
	test-ast
PRIVATE
	mint::mint
	GTest::gtest
	GTest::gtest_main
)

gtest_discover_tests(test-ast)
