# Look for the header file.
find_path(MINT_INCLUDE_DIR NAMES mint/global.h)
mark_as_advanced(MINT_INCLUDE_DIR)

# Look for the library.
find_library(MINT_LIBRARY NAMES libmint mint)
mark_as_advanced(MINT_LIBRARY)
