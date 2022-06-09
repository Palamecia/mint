project(memory)

set(${PROJECT_NAME}_BUILTIN_HDR
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/array.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/hash.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/iterator.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/libobject.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/library.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/regex.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/builtin/string.h
)

set(${PROJECT_NAME}_HDR
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/algorithm.hpp
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/casttool.h
	${MINT_INCLUDE_DIR}/${PROJECT_NAME}/class.h
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

set(${PROJECT_NAME}_BUILTIN_SRC
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

set(${PROJECT_NAME}_SRC
	casttool.cpp
	class.cpp
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

add_library(${PROJECT_NAME}
	OBJECT
	${${PROJECT_NAME}_BUILTIN_HDR}
	${${PROJECT_NAME}_BUILTIN_SRC}
	${${PROJECT_NAME}_HDR}
	${${PROJECT_NAME}_SRC}
)

if (UNIX)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# Install
install_headers(${${PROJECT_NAME}_HDR} SUBDIR ${PROJECT_NAME})
install_headers(${${PROJECT_NAME}_BUILTIN_HDR} SUBDIR ${PROJECT_NAME}/builtin)
