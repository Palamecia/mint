add_library(liburing::io_uring INTERFACE IMPORTED GLOBAL)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Search for PkgConfig
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        # Search for liburing
        pkg_check_modules(LIBURING QUIET IMPORTED_TARGET liburing)
        if(LIBURING_FOUND)
            set_target_properties(
                liburing::io_uring
            PROPERTIES
                INTERFACE_LINK_LIBRARIES PkgConfig::LIBURING
                INTERFACE_COMPILE_DEFINITIONS HAS_IO_URING=1
            )
        endif()
    endif()
endif()
