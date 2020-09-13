
#[=======================================================================[.rst:
mint-config
---------

Find the native MINT includes and library.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``MINT::MINT``, if
MINT has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

::

  MINT_INCLUDE_DIRS   - where to find MINT header files
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


# TODO handle ${MINT_ROOT}

# Look for the header file.
find_path(MINT_INCLUDE_DIR NAMES mint/config.h)
mark_as_advanced(MINT_INCLUDE_DIR)

# Look for the library.
find_library(MINT_LIBRARY NAMES libmint mint)
mark_as_advanced(MINT_LIBRARY)

# Look for the version.
find_program(MINT_EXECUTABLE NAMES mint)

if (MINT_EXECUTABLE)

	execute_process(COMMAND ${MINT_EXECUTABLE} --version
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
find_package_handle_standard_args(MINT REQUIRED_VARS MINT_LIBRARY MINT_INCLUDE_DIR VERSION_VAR MINT_VERSION_STRING)

if (MINT_FOUND)

	if (NOT TARGET MINT::MINT)
		add_library(MINT::MINT INTERFACE IMPORTED)
	endif()

	set(MINT_INCLUDE_DIRS ${MINT_INCLUDE_DIR} ${MINT_INCLUDE_DIR}/mint)
	set_property(TARGET MINT::MINT PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${MINT_INCLUDE_DIRS}")

	set(MINT_LIBRARIES ${MINT_LIBRARY})
	set_property(TARGET MINT::MINT PROPERTY INTERFACE_LINK_LIBRARIES "${MINT_LIBRARIES}")
endif()
