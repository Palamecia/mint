project(test-ast)

add_executable(test-ast)

target_sources(
	test-ast
	PRIVATE
	abstractsyntaxtree.cpp
	cursor.cpp
	fileprinter.cpp
	module.cpp
	node.cpp
	printer.cpp
)

target_link_libraries(
	test-ast
	GTest::Main
	GTest::GTest
	mint_library
)

gtest_discover_tests(test-ast)
