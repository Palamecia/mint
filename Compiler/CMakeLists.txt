project(Compiler)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(${PROJECT_NAME}_YY
    ${CMAKE_CURRENT_SOURCE_DIR}/parser.y
)

set(${PROJECT_NAME}_HPP
    ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
)

add_custom_command(
    OUTPUT ${${PROJECT_NAME}_HPP}
    DEPENDS ${${PROJECT_NAME}_YY}
    COMMAND ${BISON_EXECUTABLE}
    ARGS -y ${${PROJECT_NAME}_YY}
         -o ${${PROJECT_NAME}_HPP}
         -L C++
)

set(${PROJECT_NAME}_HDR
    ${${PROJECT_NAME}_HPP}
    buildtool.h
    compiler.h
    lexer.h
)

set(${PROJECT_NAME}_SRC
    buildtool.cpp
    compiler.cpp
    lexer.cpp
)

add_library(${PROJECT_NAME}
    ${${PROJECT_NAME}_HDR}
    ${${PROJECT_NAME}_SRC}
    ${${PROJECT_NAME}_YY}
)

find_package(BISON)
