# Path configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# C++ flags
set(CMAKE_CXX_STANDARD 17)
if (MSVC)
	add_definitions("-DNOMINMAX")
	add_definitions("-D_CRT_SECURE_NO_DEPRECATE")
	add_definitions("-D_CRT_NONSTDC_NO_DEPRECATE")
	add_definitions("/W3")

	if (MSVC_VERSION GREATER_EQUAL 1930)
		option(CXX_SANITIZE_ADDRESS "Enable address sanitizer" off)
		if (CXX_SANITIZE_ADDRESS)
			add_definitions("/fsanitize=address")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
		endif()
	endif()
else()
	add_definitions("-Wall -Wconversion")
	set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

	option(CXX_SANITIZE_ADDRESS "Enable address sanitizer" off)
	if (CXX_SANITIZE_ADDRESS)
		add_definitions("-fsanitize=address")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
	endif()

	option(CXX_SANITIZE_LEAK "Enable leak sanitizer" off)
	if (CXX_SANITIZE_LEAK)
		add_definitions("-fsanitize=leak")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
	endif()

	option(CXX_SANITIZE_THREAD "Enable thread sanitizer" off)
	if (CXX_SANITIZE_THREAD)
	        add_definitions("-fsanitize=thread -ltsan")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
	endif()
endif()

message(STATUS "Build type : ${CMAKE_BUILD_TYPE}")
