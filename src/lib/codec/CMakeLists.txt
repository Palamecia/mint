project(mint-codec)

set(MODULE_NAME codec)

find_package(Iconv REQUIRED)

add_library(mint-codec MODULE)

target_sources(
	mint-codec
PRIVATE
	iconv.cpp
	mimetypes.cpp
)

set_target_properties(
	mint-codec
	PROPERTIES
	PREFIX lib
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

if (UNIX)
	target_link_libraries(
		mint-codec
	PRIVATE
		libmint
		magic
	)
else()
	target_link_libraries(
		mint-codec
	PRIVATE
		libmint
		urlmon
		Iconv::Iconv
	)
endif()

# Install
install(
	TARGETS mint-codec
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)

add_dependencies(mint-codec module-codec)
