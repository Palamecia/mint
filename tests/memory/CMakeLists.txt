project(test-memory)

set(${PROJECT_NAME}_BUILTIN_SRC
	builtin/array.cpp
	builtin/hash.cpp
	builtin/iterator.cpp
	builtin/libobject.cpp
	builtin/library.cpp
	builtin/string.cpp
)

set(${PROJECT_NAME}_SRC
	${${PROJECT_NAME}_BUILTIN_SRC}
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

add_executable(
	${PROJECT_NAME}
	${${PROJECT_NAME}_SRC}
)

target_link_libraries(
	${PROJECT_NAME}
	${GTEST_MAIN_LIBRARY}
	${GTEST_LIBRARY}
	mint_library
)

# gtest_add_tests(TARGET ${PROJECT_NAME})
