project(test-memory)

add_executable(test-memory)

target_sources(
	test-memory
PRIVATE
	builtin/array.cpp
	builtin/hash.cpp
	builtin/iterator.cpp
	builtin/libobject.cpp
	builtin/library.cpp
	builtin/string.cpp
	cast_tools.cpp
	class.cpp
	function_tools.cpp
	garbage_collector.cpp
	global_data.cpp
	memory_tools.cpp
	object.cpp
	operator_tools.cpp
	reference.cpp
	symbol_table.cpp
)

target_link_libraries(
	test-memory
PRIVATE
	mint::mint
	GTest::gtest
	GTest::gtest_main
)

gtest_discover_tests(test-memory)
