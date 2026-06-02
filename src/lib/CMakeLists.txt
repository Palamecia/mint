project(lib)

add_subdirectory(codec)
add_subdirectory(exception)
add_subdirectory(mint)
add_subdirectory(network)
add_subdirectory(runtime)
add_subdirectory(serializer)
add_subdirectory(system)
add_subdirectory(test)

if (WIN32)
	install(
        CODE "\
        file(GLOB _libraries \"\${CMAKE_INSTALL_PREFIX}/${MINT_LIBRARY_INSTALL_DIR}/mint/*.dll\")\n\
        foreach (_library \${_libraries})\n\
            get_filename_component(_library_name \${_library} NAME)\n\
            if (NOT \${_library_name} MATCHES \"^libmint-.*\")\n\
                file(INSTALL \${_library} DESTINATION \${CMAKE_INSTALL_PREFIX}/${MINT_RUNTIME_INSTALL_DIR})\n\
            endif()\n\
        endforeach()"
        COMPONENT runtime
    )
endif()
