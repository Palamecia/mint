project(tests)

enable_testing()
include(GoogleTest)

add_subdirectory(ast)
add_subdirectory(compiler)
add_subdirectory(debug)
add_subdirectory(memory)
add_subdirectory(scheduler)
add_subdirectory(system)
