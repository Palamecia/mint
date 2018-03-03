# Path configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# C++ flags
set(CMAKE_CXX_STANDARD 11)
if (MSVC)
	add_definitions("-DNOMINMAX")
	add_definitions("-D_CRT_SECURE_NO_DEPRECATE")
	add_definitions("/W3")
else()
	add_definitions("-Wall")

	option(CXX_SANITIZE_ADDRESS "Enable address sanitizer" off)
	if (CXX_SANITIZE_ADDRESS)
		add_definitions("-fsanitize=address")
	endif()

	option(CXX_SANITIZE_LEAK "Enable leak sanitizer" off)
	if (CXX_SANITIZE_LEAK)
		add_definitions("-fsanitize=leak")
	endif()
endif()

message(STATUS "Build type : ${CMAKE_BUILD_TYPE}")
