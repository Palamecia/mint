project(Compiler)

set(${PROJECT_NAME}_HDR
    buildtool.h
    compiler.h
    lexer.h
)

set(${PROJECT_NAME}_SRC
    buildtool.cpp
    compiler.cpp
    lexer.cpp
)

set(${PROJECT_NAME}_YY
    parser.y
)

add_library(${PROJECT_NAME}
    ${${PROJECT_NAME}_HDR}
    ${${PROJECT_NAME}_SRC}
    ${${PROJECT_NAME}_YY}
)

find_package(BISON)

add_custom_command(
    TARGET ${PROJECT_NAME}
    PRE_BUILD
    COMMAND ${BISON_EXECUTABLE}
    ARGS -y ${CMAKE_CURRENT_SOURCE_DIR}/parser.y
         -o ${CMAKE_CURRENT_SOURCE_DIR}/parser.hpp
         -L C++
)
