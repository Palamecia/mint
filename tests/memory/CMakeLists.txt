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
	casttool.cpp
	class.cpp
	functiontool.cpp
	garbagecollector.cpp
	globaldata.cpp
	memorytool.cpp
	object.cpp
	operatortool.cpp
	reference.cpp
	symboltable.cpp
)

target_link_libraries(
	test-memory
	GTest::Main
	GTest::GTest
	mint_library
)

gtest_discover_tests(test-memory)
