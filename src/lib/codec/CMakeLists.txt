project(mint-codec)

find_package(Iconv REQUIRED)

include_directories(${Iconv_INCLUDE_DIRS})

add_library(${PROJECT_NAME}
	MODULE
	iconv.cpp
	mimetypes.cpp
)

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	PREFIX lib
)

if (UNIX)
	target_link_libraries(${PROJECT_NAME} mint_library magic)
else()
	target_link_libraries(${PROJECT_NAME} mint_library urlmon ${Iconv_LIBRARIES})
endif()

# Install
install_library(${PROJECT_NAME} SUBDIR mint)
