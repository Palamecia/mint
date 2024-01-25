project(test-ast)

add_executable(
	${PROJECT_NAME}
	abstractsyntaxtree.cpp
	cursor.cpp
	fileprinter.cpp
	module.cpp
	node.cpp
	printer.cpp
)

target_link_libraries(
	${PROJECT_NAME}
	GTest::gtest_main
	GTest::gtest
	mint_library
)

# gtest_add_tests(TARGET ${PROJECT_NAME})
