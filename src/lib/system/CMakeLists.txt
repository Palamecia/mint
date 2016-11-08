project(mint-system)

add_library(${PROJECT_NAME}
    MODULE
    file.cpp
)

target_link_libraries(${PROJECT_NAME} libmint)
