project(mint_library)

add_definitions(-DBUILD_MINT_LIB)

if (WIN32)
	find_path(ICU_INCLUDE_DIR NAMES icu.h)
else()
	find_package(ICU QUIET uc)
endif()

if (ICU_INCLUDE_DIR)
	message(STATUS "International Components for Unicode (ICU) - enabled")
	include_directories(${ICU_INCLUDE_DIR})
	add_definitions(-DMINT_UTF8_WITH_ICU)
else()
	message(STATUS "International Components for Unicode (ICU) - disabled")
endif()

add_subdirectory(ast)
add_subdirectory(compiler)
add_subdirectory(debug)
add_subdirectory(memory)
add_subdirectory(scheduler)
add_subdirectory(system)

# Target mint_library
add_library(
	${PROJECT_NAME}
	SHARED
	$<TARGET_OBJECTS:ast>
	$<TARGET_OBJECTS:compiler>
	$<TARGET_OBJECTS:debug>
	$<TARGET_OBJECTS:memory>
	$<TARGET_OBJECTS:scheduler>
	$<TARGET_OBJECTS:system>
)

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME mint
	PREFIX lib
)

if (ICU_INCLUDE_DIR)
	target_link_libraries(${PROJECT_NAME} icu)
endif()

if (UNIX)
	target_link_libraries(
		${PROJECT_NAME}
		pthread
		dl
	)
elseif (WIN32)
	target_link_libraries(
		${PROJECT_NAME}
		userenv
	)
endif()

add_dependencies(${PROJECT_NAME} scheduler)

# Install
install_library(${PROJECT_NAME})
