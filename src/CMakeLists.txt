project(src)

if (WIN32)
	find_path(ICU_INCLUDE_DIR NAMES icu.h)
	if (ICU_INCLUDE_DIR)
		set(ICU_LIBRARIES icu)
	endif()
else()
	find_package(ICU QUIET COMPONENTS uc)
endif()

if (ICU_INCLUDE_DIR)
	message(STATUS "International Components for Unicode (ICU) - enabled")
	include_directories(${ICU_INCLUDE_DIR})
	add_definitions(-DMINT_WITH_ICU)
else()
	message(STATUS "International Components for Unicode (ICU) - disabled")
endif()

add_subdirectory(lib)
add_subdirectory(libmint)

add_subdirectory(mdbg)
add_subdirectory(mint)
add_subdirectory(mintdoc)
