project(tests)

enable_testing()
include(GoogleTest)

if (NOT TARGET GTest::GTest)
	add_library(GTest::GTest SHARED IMPORTED)
	set_target_properties(GTest::GTest PROPERTIES
		IMPORTED_LOCATION ${GTEST_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIRS}
	)
endif()

if (NOT TARGET GTest::Main)
	add_library(GTest::Main SHARED IMPORTED)
	set_target_properties(GTest::Main PROPERTIES
		IMPORTED_LOCATION ${GTEST_MAIN_LIBRARY}
		INTERFACE_LINK_LIBRARIES GTest::GTest
	)
endif()

add_subdirectory(ast)
add_subdirectory(compiler)
add_subdirectory(debug)
add_subdirectory(memory)
add_subdirectory(scheduler)
add_subdirectory(system)
