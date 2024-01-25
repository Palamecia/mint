project(test-system)

add_executable(
	${PROJECT_NAME}
	bufferstream.cpp
	datastream.cpp
	error.cpp
	filestream.cpp
	filesystem.cpp
	plugin.cpp
	terminal.cpp
	utf8.cpp
)

target_link_libraries(
	${PROJECT_NAME}
	GTest::gtest_main
	GTest::gtest
	mint_library
)

# gtest_add_tests(TARGET ${PROJECT_NAME})
