project(mint-codec)

set(MODULE_NAME codec)

find_package(Iconv REQUIRED)

include_directories(${Iconv_INCLUDE_DIRS})

set(${PROJECT_NAME}_SOURCES
	iconv.cpp
	mimetypes.cpp
)

add_library(${PROJECT_NAME}
	MODULE
	${${PROJECT_NAME}_SOURCES}
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
	get_filename_component(Iconv_LIBRARIES_DIR "${Iconv_LIBRARIES}" DIRECTORY)
	get_filename_component(Iconv_DLL "${Iconv_LIBRARIES_DIR}/../bin/iconv.dll" ABSOLUTE)
	install(PROGRAMS "${Iconv_DLL}" COMPONENT runtime DESTINATION ${MINT_RUNTIME_INSTALL_DIR})
endif()

# Install
install_library(${PROJECT_NAME} SUBDIR mint)
