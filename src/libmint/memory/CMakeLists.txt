project(memory)

add_library(memory OBJECT)

set(MEMORY_BUILTIN_HEADERS
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/array.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/hash.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/iterator.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/libobject.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/library.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/regex.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/string.h
)

set(MEMORY_HEADERS
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
)

set(MEMORY_BUILTIN_SOURCES
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
)

set(MEMORY_SOURCES
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

target_sources(
	memory
	PUBLIC
	${MEMORY_BUILTIN_HEADERS}
	${MEMORY_HEADERS}
	PRIVATE
	${MEMORY_BUILTIN_SOURCES}
	${MEMORY_SOURCES}
)

source_group("Header Files Builtin" FILES ${MEMORY_BUILTIN_HEADERS})
source_group("Source Files Builtin" FILES ${MEMORY_BUILTIN_SOURCES})

if (UNIX)
	set_target_properties(memory PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
mint_install_headers(${MEMORY_HEADERS} SUBDIR ${PROJECT_NAME})
mint_install_headers(${MEMORY_BUILTIN_HEADERS} SUBDIR ${PROJECT_NAME}/builtin)
