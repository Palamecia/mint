project(test-memory)

add_executable(
	${PROJECT_NAME}
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
	${PROJECT_NAME}
	GTest::gtest_main
	GTest::gtest
	mint_library
)

# gtest_add_tests(TARGET ${PROJECT_NAME})
