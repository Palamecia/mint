project(Memory)

set(${PROJECT_NAME}_HDR
    casttool.h
    class.h
    garbagecollector.h
    globaldata.h
    memorytool.h
    object.h
    operatortool.h
    reference.h
    symboltable.h
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
