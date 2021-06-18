project(tests)

enable_testing()

if (NOT TARGET GTest::gtest)
	add_library(GTest::gtest SHARED IMPORTED)
	set_target_properties(GTest::gtest PROPERTIES
		IMPORTED_LOCATION ${GTEST_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIRS}
	)
endif()

if (NOT TARGET GTest::gtest_main)
	add_library(GTest::gtest_main SHARED IMPORTED)
	set_target_properties(GTest::gtest_main PROPERTIES
		IMPORTED_LOCATION ${GTEST_MAIN_LIBRARY}
		INTERFACE_LINK_LIBRARIES GTest::gtest
	)
endif()

add_subdirectory(ast)
add_subdirectory(compiler)
add_subdirectory(debug)
add_subdirectory(memory)
add_subdirectory(scheduler)
add_subdirectory(system)

add_test(
	NAME test
	COMMAND test.mn
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
