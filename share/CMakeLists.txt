project(share)

# TODO install syntax file
add_custom_target(syntax
	SOURCES
	mint.xml
)

install_cmake(cmake/mint-config.cmake)
