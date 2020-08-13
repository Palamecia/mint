project(share)

# TODO install syntax file
add_custom_target(kate-syntax
	SOURCES
	kate/mint.xml
)
add_custom_target(subl-syntax
	SOURCES
	subl/Mint.sublime-syntax
)
add_custom_target(vscode-syntax
	SOURCES
	vscode/syntaxes/mint.tmLanguage
)

install_cmake(cmake/mint-config.cmake)
