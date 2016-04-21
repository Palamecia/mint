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

add_library(${PROJECT_NAME}
    ${${PROJECT_NAME}_HDR}
    ${${PROJECT_NAME}_SRC}
)
