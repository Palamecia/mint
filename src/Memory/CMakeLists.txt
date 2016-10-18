project(Memory)

set(${PROJECT_NAME}_HDR
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/casttool.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/class.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/garbagecollector.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/globaldata.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/memorytool.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/object.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/operatortool.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/reference.h
    ${MINT_INCLUDE_DIR}/${PROJECT_NAME}/symboltable.h
)

set(${PROJECT_NAME}_SRC
    casttool.cpp
    class.cpp
    garbagecollector.cpp
    globaldata.cpp
    memorytool.cpp
    object.cpp
    operatortool.cpp
    reference.cpp
    symboltable.cpp
)

set(${PROJECT_NAME}_BUILTIN_SRC
    builtin/array.cpp
    builtin/hash.cpp
    builtin/iterator.cpp
    builtin/string.cpp
)

add_library(${PROJECT_NAME}
    ${${PROJECT_NAME}_HDR}
    ${${PROJECT_NAME}_SRC}
    ${${PROJECT_NAME}_BUILTIN_SRC}
)
