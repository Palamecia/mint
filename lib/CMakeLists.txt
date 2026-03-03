project(lib)

option(MINT_COPY_MODULE_SOURCES_TO_BUILD_DIR "Copy library module source files into CMAKE_LIBRARY_OUTPUT_DIRECTORY during build" OFF)

# Helper to expose per-module source copy behavior across lib subdirectories.
# Attach copy logic directly to the module target; no user-visible extra custom target required.
function(mint_copy_module_sources target module_subdir)
    if(NOT MINT_COPY_MODULE_SOURCES_TO_BUILD_DIR)
        return()
    endif()

    get_target_property(files ${target} SOURCES)
    if(NOT files)
      return()
    endif()

    set(module_output_dir "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${module_subdir}")

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${module_output_dir}"
    )

    foreach(src IN LISTS files)
        set(src_path "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
        set(dst_path "${module_output_dir}/${src}")

        get_filename_component(dst_dir "${dst_path}" DIRECTORY)

        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${dst_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${src_path}" "${dst_path}"
            VERBATIM
        )
    endforeach()
endfunction()

add_subdirectory(codec)
add_subdirectory(container)
add_subdirectory(exception)
add_subdirectory(event)
add_subdirectory(mint)
add_subdirectory(network)
add_subdirectory(serializer)
add_subdirectory(system)
add_subdirectory(test)
