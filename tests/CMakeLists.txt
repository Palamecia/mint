project(tests)

enable_testing()

add_test(
	NAME test-operator
	COMMAND test-operator.mn
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
