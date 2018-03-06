project(share)

add_custom_target(syntax
	SOURCES
	mint.xml
)

install_cmake(cmake/flags.cmake)
install_cmake(cmake/install.cmake)
# install_cmake(cmake/mint-config.cmake)
