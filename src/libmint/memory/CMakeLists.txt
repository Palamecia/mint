project(memory)

add_library(memory OBJECT)

target_sources(
	memory
	PUBLIC
	FILE_SET builtin TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/array.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/hash.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/iterator.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/libobject.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/library.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/regex.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/string.h
	FILE_SET HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/algorithm.hpp
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/casttool.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/class.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/classtool.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/data.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/functiontool.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/garbagecollector.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/globaldata.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/memorypool.hpp
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/memorytool.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/object.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/objectprinter.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/operatortool.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/reference.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/symboltable.h
	PRIVATE
	builtin/array.cpp
	builtin/hash.cpp
	builtin/iterator.cpp
	builtin/iterator_items.cpp
	builtin/iterator_range.cpp
	builtin/iterator_generator.cpp
	builtin/libobject.cpp
	builtin/library.cpp
	builtin/regex.cpp
	builtin/string.cpp
	casttool.cpp
	class.cpp
	classtool.cpp
	data.cpp
	functiontool.cpp
	garbagecollector.cpp
	globaldata.cpp
	memorytool.cpp
	object.cpp
	objectprinter.cpp
	operatortool.cpp
	reference.cpp
	symboltable.cpp
)

source_group(TREE ${MINT_INCLUDE_DIR}/${PROJECT_NAME})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR})

if (UNIX)
	set_target_properties(memory PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
mint_install_headers($<TARGET_PROPERTY:memory,HEADER_SET> SUBDIR ${PROJECT_NAME})
mint_install_headers($<TARGET_PROPERTY:memory,HEADER_SET_builtin> SUBDIR ${PROJECT_NAME}/builtin)
