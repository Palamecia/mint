project(mint-system)

add_library(${PROJECT_NAME}
    MODULE
    file.cpp
	process.cpp
)

target_link_libraries(${PROJECT_NAME} libmint)
