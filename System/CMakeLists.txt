project(System)

set(${PROJECT_NAME}_HDR
    datastream.h
    error.h
    filestream.h
    inputstream.h
    output.h
    printer.h
    utf8iterator.h
)

set(${PROJECT_NAME}_SRC
    datastream.cpp
    error.cpp
    filestream.cpp
    inputstream.cpp
    output.cpp
    printer.cpp
    utf8iterator.cpp
)

add_library(${PROJECT_NAME}
    ${${PROJECT_NAME}_HDR}
    ${${PROJECT_NAME}_SRC}
)

# target_link_libraries(${PROJECT_NAME}
#     readline
# )
