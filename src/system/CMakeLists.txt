project(system)

set(${PROJECT_NAME}_HDR
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/datastream.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/error.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/filestream.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/filesystem.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/inputstream.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/output.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/plugin.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/printer.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/utf8iterator.h
)

set(${PROJECT_NAME}_SRC
    datastream.cpp
    error.cpp
    filestream.cpp
    filesystem.cpp
    inputstream.cpp
    output.cpp
    plugin.cpp
    printer.cpp
    utf8iterator.cpp
)

add_library(${PROJECT_NAME}
    ${${PROJECT_NAME}_HDR}
    ${${PROJECT_NAME}_SRC}
)

if (UNIX)
    target_link_libraries(${PROJECT_NAME}
        # readline
        dl
    )
endif()
