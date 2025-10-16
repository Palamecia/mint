project(libmint)

find_package(BISON REQUIRED)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
	DEPENDS compiler/parser.y
	COMMAND ${BISON_EXECUTABLE}
	ARGS -y ${CMAKE_CURRENT_SOURCE_DIR}/compiler/parser.y
	     -o ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
		 -L C++
		 -v
	COMMENT "Generating file ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp"
)

find_package(ICU REQUIRED COMPONENTS uc i18n data)
find_package(Microsoft.GSL CONFIG REQUIRED)

# Target libmint
add_library(libmint SHARED)

target_compile_definitions(
	libmint
PRIVATE
	-DBUILD_MINT_LIB
)

option(MINT_DUMP_ASSEMBLY "Dump compiled assembly on debug" ON)
if (MINT_DUMP_ASSEMBLY)
	target_compile_definitions(
		libmint
	PRIVATE
		-DMINT_DUMP_ASSEMBLY
	)
endif()

target_include_directories(
	libmint
PUBLIC
	$<BUILD_INTERFACE:${MINT_INCLUDE_DIR}>
	$<INSTALL_INTERFACE:${MINT_HEADERS_INSTALL_DIR}>
PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}
)

target_sources(
	libmint
PUBLIC
	FILE_SET ast TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES
	${MINT_INCLUDE_DIR}/mint/ast/abstractsyntaxtree.h
	${MINT_INCLUDE_DIR}/mint/ast/abstractsyntaxtreewalker.h
	${MINT_INCLUDE_DIR}/mint/ast/asttools.h
	${MINT_INCLUDE_DIR}/mint/ast/classregister.h
	${MINT_INCLUDE_DIR}/mint/ast/cursor.h
	${MINT_INCLUDE_DIR}/mint/ast/fileprinter.h
	${MINT_INCLUDE_DIR}/mint/ast/module.h
	${MINT_INCLUDE_DIR}/mint/ast/node.h
	${MINT_INCLUDE_DIR}/mint/ast/printer.h
	${MINT_INCLUDE_DIR}/mint/ast/savedstate.h
	${MINT_INCLUDE_DIR}/mint/ast/symbol.h
PRIVATE
	ast/abstractsyntaxtree.cpp
	ast/asttools.cpp
	ast/classregister.cpp
	ast/cursor.cpp
	ast/fileprinter.cpp
	ast/module.cpp
	ast/node.cpp
	ast/printer.cpp
	ast/savedstate.cpp
	ast/symbol.cpp
	ast/threadentrypoint.cpp
	ast/threadentrypoint.h
)

target_sources(
	libmint
PUBLIC
	FILE_SET compiler TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES
	${MINT_INCLUDE_DIR}/mint/compiler/buildtool.h
	${MINT_INCLUDE_DIR}/mint/compiler/compiler.h
	${MINT_INCLUDE_DIR}/mint/compiler/lexicalhandler.h
	${MINT_INCLUDE_DIR}/mint/compiler/lexer.h
	${MINT_INCLUDE_DIR}/mint/compiler/token.h
PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
	compiler/block.cpp
	compiler/block.h
	compiler/branch.cpp
	compiler/branch.h
	compiler/buildtool.cpp
	compiler/casetable.cpp
	compiler/casetable.h
	compiler/catchcontext.cpp
	compiler/catchcontext.h
	compiler/compiler.cpp
	compiler/context.cpp
	compiler/context.h
	compiler/lexer.cpp
	compiler/lexicalhandler.cpp
	compiler/parser.y
)

target_sources(
	libmint
PUBLIC
	FILE_SET debug TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES
	${MINT_INCLUDE_DIR}/mint/debug/cursordebugger.h
	${MINT_INCLUDE_DIR}/mint/debug/debuginfo.h
	${MINT_INCLUDE_DIR}/mint/debug/debuginterface.h
	${MINT_INCLUDE_DIR}/mint/debug/debugtool.h
	${MINT_INCLUDE_DIR}/mint/debug/lineinfo.h
	${MINT_INCLUDE_DIR}/mint/debug/threadcontext.h
PRIVATE
	debug/cursordebugger.cpp
	debug/debuginfo.cpp
	debug/debuginterface.cpp
	debug/debugtool.cpp
	debug/lineinfo.cpp
)

target_sources(
	libmint
PUBLIC
	FILE_SET memory_builtin TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES
	${MINT_INCLUDE_DIR}/mint/memory/builtin/array.h
	${MINT_INCLUDE_DIR}/mint/memory/builtin/hash.h
	${MINT_INCLUDE_DIR}/mint/memory/builtin/iterator.h
	${MINT_INCLUDE_DIR}/mint/memory/builtin/libobject.h
	${MINT_INCLUDE_DIR}/mint/memory/builtin/library.h
	${MINT_INCLUDE_DIR}/mint/memory/builtin/regex.h
	${MINT_INCLUDE_DIR}/mint/memory/builtin/string.h
	FILE_SET memory TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES
	${MINT_INCLUDE_DIR}/mint/memory/algorithm.hpp
	${MINT_INCLUDE_DIR}/mint/memory/casttool.h
	${MINT_INCLUDE_DIR}/mint/memory/class.h
	${MINT_INCLUDE_DIR}/mint/memory/classtool.h
	${MINT_INCLUDE_DIR}/mint/memory/data.h
	${MINT_INCLUDE_DIR}/mint/memory/functiontool.h
	${MINT_INCLUDE_DIR}/mint/memory/garbagecollector.h
	${MINT_INCLUDE_DIR}/mint/memory/globaldata.h
	${MINT_INCLUDE_DIR}/mint/memory/memorypool.hpp
	${MINT_INCLUDE_DIR}/mint/memory/memorytool.h
	${MINT_INCLUDE_DIR}/mint/memory/object.h
	${MINT_INCLUDE_DIR}/mint/memory/objectprinter.h
	${MINT_INCLUDE_DIR}/mint/memory/operatortool.h
	${MINT_INCLUDE_DIR}/mint/memory/reference.h
	${MINT_INCLUDE_DIR}/mint/memory/symboltable.h
PRIVATE
	memory/builtin/array.cpp
	memory/builtin/hash.cpp
	memory/builtin/iterator.cpp
	memory/builtin/iterator_items.cpp
	memory/builtin/iterator_range.cpp
	memory/builtin/iterator_generator.cpp
	memory/builtin/libobject.cpp
	memory/builtin/library.cpp
	memory/builtin/regex.cpp
	memory/builtin/string.cpp
	memory/casttool.cpp
	memory/class.cpp
	memory/classtool.cpp
	memory/data.cpp
	memory/functiontool.cpp
	memory/garbagecollector.cpp
	memory/globaldata.cpp
	memory/memorytool.cpp
	memory/object.cpp
	memory/objectprinter.cpp
	memory/operatortool.cpp
	memory/reference.cpp
	memory/symboltable.cpp
)

target_sources(
	libmint
PUBLIC
	FILE_SET scheduler TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES
	${MINT_INCLUDE_DIR}/mint/scheduler/destructor.h
	${MINT_INCLUDE_DIR}/mint/scheduler/exception.h
	${MINT_INCLUDE_DIR}/mint/scheduler/generator.h
	${MINT_INCLUDE_DIR}/mint/scheduler/inputstream.h
	${MINT_INCLUDE_DIR}/mint/scheduler/output.h
	${MINT_INCLUDE_DIR}/mint/scheduler/process.h
	${MINT_INCLUDE_DIR}/mint/scheduler/processor.h
	${MINT_INCLUDE_DIR}/mint/scheduler/scheduler.h
	${MINT_INCLUDE_DIR}/mint/scheduler/threadpool.h
PRIVATE
	scheduler/bracematcher.cpp
	scheduler/bracematcher.h
	scheduler/completer.cpp
	scheduler/completer.h
	scheduler/destructor.cpp
	scheduler/exception.cpp
	scheduler/generator.cpp
	scheduler/highlighter.cpp
	scheduler/highlighter.h
	scheduler/inputstream.cpp
	scheduler/output.cpp
	scheduler/process.cpp
	scheduler/processor.cpp
	scheduler/scheduler.cpp
	scheduler/threadpool.cpp
)

target_sources(
	libmint
PUBLIC
	FILE_SET system TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES
	${MINT_INCLUDE_DIR}/mint/system/arguments.h
	${MINT_INCLUDE_DIR}/mint/system/assert.h
	${MINT_INCLUDE_DIR}/mint/system/bufferstream.h
	${MINT_INCLUDE_DIR}/mint/system/datastream.h
	${MINT_INCLUDE_DIR}/mint/system/errno.h
	${MINT_INCLUDE_DIR}/mint/system/error.h
	${MINT_INCLUDE_DIR}/mint/system/filestream.h
	${MINT_INCLUDE_DIR}/mint/system/filesystem.h
	${MINT_INCLUDE_DIR}/mint/system/mintruntimeerror.hpp
	${MINT_INCLUDE_DIR}/mint/system/pipe.h
	${MINT_INCLUDE_DIR}/mint/system/plugin.h
	${MINT_INCLUDE_DIR}/mint/system/poolallocator.hpp
	${MINT_INCLUDE_DIR}/mint/system/stdio.h
	${MINT_INCLUDE_DIR}/mint/system/string.h
	${MINT_INCLUDE_DIR}/mint/system/terminal.h
	${MINT_INCLUDE_DIR}/mint/system/utf8.h
PRIVATE
	system/arguments.cpp
	system/assert.cpp
	system/bufferstream.cpp
	system/datastream.cpp
	system/errno.cpp
	system/error.cpp
	system/filestream.cpp
	system/filesystem.cpp
	system/pipe.cpp
	system/plugin.cpp
	system/stdio.cpp
	system/string.cpp
	system/terminal.cpp
	system/utf8.cpp
)

if (WIN32)
	target_sources(
		libmint
	PRIVATE
		system/win32/globalsid.cpp
		system/win32/globalsid.h
		system/win32/pipe.cpp
		system/win32/pipe.h
		system/win32/terminal.cpp
		system/win32/terminal.h
	)
else()
	target_sources(
		libmint
	PRIVATE
		system/unix/terminal.cpp
		system/unix/terminal.h
	)
endif()

target_sources(
	libmint
PUBLIC
	FILE_SET config TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES ${MINT_INCLUDE_DIR}/mint/config.h
)

set_target_properties(
	libmint
	PROPERTIES
	OUTPUT_NAME mint
	PREFIX lib
	VERIFY_INTERFACE_HEADER_SETS ON
)

target_link_libraries(
	libmint
PUBLIC
	Microsoft.GSL::GSL
PRIVATE
	ICU::i18n
	ICU::uc
	ICU::data
)

if (UNIX)
	target_link_libraries(
		libmint
	PRIVATE
		pthread
		dl
	)
elseif (WIN32)
	target_link_libraries(
		libmint
	PRIVATE
		userenv
	)
endif()

# Install
install(
	TARGETS libmint
	EXPORT libmint-targets
	INCLUDES DESTINATION ${MINT_HEADERS_INSTALL_DIR}
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}
	RUNTIME COMPONENT runtime DESTINATION ${MINT_RUNTIME_INSTALL_DIR}
	FILE_SET ast COMPONENT devel DESTINATION ${MINT_HEADERS_INSTALL_DIR}
	FILE_SET compiler COMPONENT devel DESTINATION ${MINT_HEADERS_INSTALL_DIR}
	FILE_SET debug COMPONENT devel DESTINATION ${MINT_HEADERS_INSTALL_DIR}
	FILE_SET memory COMPONENT devel DESTINATION ${MINT_HEADERS_INSTALL_DIR}
	FILE_SET memory_builtin COMPONENT devel DESTINATION ${MINT_HEADERS_INSTALL_DIR}
	FILE_SET scheduler COMPONENT devel DESTINATION ${MINT_HEADERS_INSTALL_DIR}
	FILE_SET system COMPONENT devel DESTINATION ${MINT_HEADERS_INSTALL_DIR}
	FILE_SET config COMPONENT devel DESTINATION ${MINT_HEADERS_INSTALL_DIR}
)

install(
	EXPORT libmint-targets
	FILE libmint-targets.cmake
	NAMESPACE mint::
	COMPONENT devel DESTINATION ${MINT_CMAKE_INSTALL_DIR}/mint
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
	${MINT_SHARE_DIR}/cmake/mint-config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/mint-config.cmake
	INSTALL_DESTINATION share/cmake/mint
)

write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/mint-config-version.cmake
	VERSION ${CMAKE_PROJECT_VERSION}
	COMPATIBILITY AnyNewerVersion
)

install(
	FILES
	${MINT_SHARE_DIR}/cmake/mint-flags.cmake
	${MINT_SHARE_DIR}/cmake/mint-install.cmake
	${CMAKE_CURRENT_BINARY_DIR}/mint-config.cmake
	${CMAKE_CURRENT_BINARY_DIR}/mint-config-version.cmake
	COMPONENT devel DESTINATION ${MINT_CMAKE_INSTALL_DIR}/mint
)
