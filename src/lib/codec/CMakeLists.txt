project(mint-codec)

find_package(Iconv REQUIRED)
find_package(ICU REQUIRED COMPONENTS uc i18n data)

add_library(mint-lib-module-codec MODULE)

set_target_properties(
	mint-lib-module-codec
	PROPERTIES
	PREFIX lib
	OUTPUT_NAME mint-codec
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_sources(
	mint-lib-module-codec
PRIVATE
	iconv.cpp
	mime_types.cpp
	text_codec.cpp
)

target_link_libraries(
	mint-lib-module-codec
PRIVATE
	mint::mint
	ICU::i18n
	ICU::uc
	ICU::data
)

if (UNIX)
	target_link_libraries(
		mint-lib-module-codec
	PRIVATE
		magic
	)
else()
	target_link_libraries(
		mint-lib-module-codec
	PRIVATE
		urlmon
		Iconv::Iconv
	)
endif()

# Install
install(
	TARGETS mint-lib-module-codec
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)

add_dependencies(mint-lib-module-codec mint-module-codec)
