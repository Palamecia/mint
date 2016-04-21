project(System)

set(${PROJECT_NAME}_HDR
    datastream.h
    filestream.h
    printer.h
)

set(${PROJECT_NAME}_SRC
    datastream.cpp
    filestream.cpp
    printer.cpp
)

add_library(${PROJECT_NAME}
    ${${PROJECT_NAME}_HDR}
    ${${PROJECT_NAME}_SRC}
)
