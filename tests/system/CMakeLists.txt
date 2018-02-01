project(test-system)

set(${PROJECT_NAME}_SRC
	bufferstream.cpp
	datastream.cpp
	error.cpp
	filestream.cpp
	filesystem.cpp
	inputstream.cpp
	output.cpp
	plugin.cpp
	printer.cpp
	terminal.cpp
	utf8iterator.cpp
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
