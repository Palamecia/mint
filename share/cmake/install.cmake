include(CMakeParseArguments)

option(USE_RUNTIME_INSTALL_PREFIX "Use runtime installation directory" ON)

# Path configuration
if (UNIX)
	if (USE_RUNTIME_INSTALL_PREFIX)
		set(CMAKE_INSTALL_PREFIX "/")
	endif()
	set(MINT_RUNTIME_INSTALL_DIR "bin")
	set(MINT_LIBRARY_INSTALL_DIR "lib64")
	set(MINT_HEADERS_INSTALL_DIR "usr/include/mint")
	set(MINT_MODULES_INSTALL_DIR "${MINT_LIBRARY_INSTALL_DIR}/mint")
	set(MINT_CMAKE_INSTALL_DIR "usr/share/cmake/mint")
else()
	if (USE_RUNTIME_INSTALL_PREFIX)
		set(CMAKE_INSTALL_PREFIX "C:/")
	endif()
	set(MINT_RUNTIME_INSTALL_DIR "mint/bin")
	set(MINT_LIBRARY_INSTALL_DIR "mint/lib")
	set(MINT_HEADERS_INSTALL_DIR "mint/include")
	set(MINT_MODULES_INSTALL_DIR "${MINT_LIBRARY_INSTALL_DIR}/mint")
	set(MINT_CMAKE_INSTALL_DIR "mint/share/cmake/mint")
endif()

# Install exeutable target
function(install_executable)
	cmake_parse_arguments(
		INSTALL
		""
		"SUBDIR"
		""
		${ARGN}
	)
    if (INSTALL_SUBDIR)
		set(destination ${MINT_RUNTIME_INSTALL_DIR}/${INSTALL_SUBDIR})
	else()
		set(destination ${MINT_RUNTIME_INSTALL_DIR})
	endif()
	install(
		TARGETS ${INSTALL_UNPARSED_ARGUMENTS}
		RUNTIME COMPONENT runtime DESTINATION ${destination}
	)
endfunction()

# Install library target
function(install_library)
	cmake_parse_arguments(
		INSTALL
		""
		"SUBDIR"
		""
		${ARGN}
	)
    if (INSTALL_SUBDIR)
		set(destination ${MINT_LIBRARY_INSTALL_DIR}/${INSTALL_SUBDIR})
		set(runtime_destination ${MINT_RUNTIME_INSTALL_DIR}/${INSTALL_SUBDIR})
	else()
		set(destination ${MINT_LIBRARY_INSTALL_DIR})
		set(runtime_destination ${MINT_RUNTIME_INSTALL_DIR})
	endif()
	install(
		TARGETS ${INSTALL_UNPARSED_ARGUMENTS}
		LIBRARY COMPONENT runtime DESTINATION ${destination}
		ARCHIVE COMPONENT runtime DESTINATION ${destination}
		RUNTIME COMPONENT runtime DESTINATION ${runtime_destination}
	)
endfunction()

# Install headers files
function(install_headers)
	cmake_parse_arguments(
		INSTALL
		""
		"SUBDIR"
		""
		${ARGN}
	)
    if (INSTALL_SUBDIR)
		set(destination ${MINT_HEADERS_INSTALL_DIR}/${INSTALL_SUBDIR})
	else()
		set(destination ${MINT_HEADERS_INSTALL_DIR})
	endif()
	install(
		FILES ${INSTALL_UNPARSED_ARGUMENTS}
		COMPONENT devel DESTINATION ${destination}
	)
endfunction()

# Install mint modules files
function(install_modules)
	cmake_parse_arguments(
		INSTALL
		""
		"SUBDIR"
		""
		${ARGN}
	)
    if (INSTALL_SUBDIR)
		set(destination ${MINT_MODULES_INSTALL_DIR}/${INSTALL_SUBDIR})
	else()
		set(destination ${MINT_MODULES_INSTALL_DIR})
	endif()
	install(
		FILES ${INSTALL_UNPARSED_ARGUMENTS}
		COMPONENT runtime DESTINATION ${destination}
	)
endfunction()

# Install cmake files
function(install_cmake)
	cmake_parse_arguments(
		INSTALL
		""
		"SUBDIR"
		""
		${ARGN}
	)
    if (INSTALL_SUBDIR)
		set(destination ${MINT_CMAKE_INSTALL_DIR}/${INSTALL_SUBDIR})
	else()
		set(destination ${MINT_CMAKE_INSTALL_DIR})
	endif()
	install(
		FILES ${INSTALL_UNPARSED_ARGUMENTS}
		COMPONENT devel DESTINATION ${destination}
	)
endfunction()
