
#[=======================================================================[.rst:
mint-config
---------

Find the native MINT includes and library.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``mint::libmint``, if
MINT has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

::

  MINT_INCLUDE_DIRS   - Where to find MINT header files.
  MINT_LIBRARIES      - List of libraries when using MINT.
  MINT_FOUND          - True if MINT found.

::

  MINT_VERSION_STRING - The version of libmint found (x.y.z)
  MINT_VERSION_MAJOR  - The major version of libmint
  MINT_VERSION_MINOR  - The minor version of libmint
  MINT_VERSION_PATCH  - The patch version of libmint
  MINT_VERSION_TWEAK  - The tweak version of libmint

Hints
^^^^^

A user may set ``MINT_ROOT`` to a libmint installation root to tell this
module where to look.
#]=======================================================================]

if (WIN32)
	get_filename_component(_mint_default_root_dir "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
	set(_mint_default_include_dir include)
else()
	get_filename_component(_mint_default_root_dir "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
	set(_mint_default_include_dir include/mint)
endif()

# Look for the header file.
find_path(mint_INCLUDE_DIR
	NAMES mint/config.h
	HINTS
	    ${_mint_default_root_dir}/${_mint_default_include_dir}
		$ENV{MINT_ROOT}/${_mint_default_include_dir}
		${MINT_ROOT}/${_mint_default_include_dir}
)
mark_as_advanced(mint_INCLUDE_DIR)

# Look for the library.
find_library(mint_LIBRARY
	NAMES libmint mint
	HINTS
	    ${_mint_default_root_dir}
		ENV MINT_ROOT
		${MINT_ROOT}
)
mark_as_advanced(mint_LIBRARY)

# Look for the version.
find_program(mint_EXECUTABLE
	NAMES mint
	HINTS
	    ${_mint_default_root_dir}
		ENV MINT_ROOT
		${MINT_ROOT}
)

if (mint_EXECUTABLE)

	execute_process(COMMAND ${mint_EXECUTABLE} --version
		OUTPUT_VARIABLE MINT_VERSION_STRING
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

    string(REGEX REPLACE "^mint ([0-9]+).*$" "\\1" MINT_VERSION_MAJOR "${MINT_VERSION_STRING}")
    string(REGEX REPLACE "^mint [0-9]+\\.([0-9]+).*$" "\\1" MINT_VERSION_MINOR  "${MINT_VERSION_STRING}")
    string(REGEX REPLACE "^mint [0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" MINT_VERSION_PATCH "${MINT_VERSION_STRING}")
    string(REGEX REPLACE "^mint [0-9]+\\.[0-9]+\\.[0-9]+(.*)$" "\\1" MINT_VERSION_TWEAK "${MINT_VERSION_STRING}")
    set(MINT_VERSION_STRING "${MINT_VERSION_MAJOR}.${MINT_VERSION_MINOR}.${MINT_VERSION_PATCH}${MINT_VERSION_TWEAK}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(mint REQUIRED_VARS mint_LIBRARY mint_INCLUDE_DIR VERSION_VAR MINT_VERSION_STRING)

if (mint_FOUND)

	include({CMAKE_CURRENT_LIST_DIR}/mint-flags.cmake)
	include({CMAKE_CURRENT_LIST_DIR}/mint-install.cmake)

	if (NOT TARGET mint::libmint)
		add_library(mint::libmint INTERFACE IMPORTED)
	endif()

	set(MINT_INCLUDE_DIRS ${mint_INCLUDE_DIR} CACHE PATH "Where to find MINT header files.")
	set_property(TARGET mint::libmint PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${MINT_INCLUDE_DIRS}")

	set(MINT_LIBRARIES ${mint_LIBRARY} CACHE FILEPATH "List of libraries when using MINT.")
	set_property(TARGET mint::libmint PROPERTY INTERFACE_LINK_LIBRARIES "${MINT_LIBRARIES}")

	set(MINT_FOUND ${mint_FOUND} CACHE BOOL "True if MINT found.")
endif()
