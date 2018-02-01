project(test-scheduler)

set(${PROJECT_NAME}_SRC
	destructor.cpp
	process.cpp
	processor.cpp
	scheduler.cpp
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
