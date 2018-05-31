project(tests)

enable_testing()

include_directories(${GTEST_INCLUDE_DIR})
add_subdirectory(ast)
add_subdirectory(compiler)
add_subdirectory(memory)
add_subdirectory(scheduler)
add_subdirectory(system)

add_test(
	NAME test-operator
	COMMAND test-operator.mn
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
	NAME test-thread
	COMMAND test-thread.mn
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
