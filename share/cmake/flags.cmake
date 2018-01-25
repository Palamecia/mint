# Path configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# C++ flags
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

option(CXX_SANITIZE_ADDRESS "Enable address sanitizer" off)
if (CXX_SANITIZE_ADDRESS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

option(CXX_SANITIZE_LEAK "Enable leak sanitizer" off)
if (CXX_SANITIZE_LEAK)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
endif()

message(STATUS "Build type : ${CMAKE_BUILD_TYPE}")
