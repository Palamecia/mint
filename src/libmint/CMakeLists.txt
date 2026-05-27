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
add_library(mint-libmint SHARED)
add_library(mint::mint ALIAS mint-libmint)

set_target_properties(
	mint-libmint
	PROPERTIES
	PREFIX lib
	OUTPUT_NAME mint
	EXPORT_NAME mint
	VERIFY_INTERFACE_HEADER_SETS ON
)

target_compile_definitions(
	mint-libmint
PRIVATE
	-DBUILD_MINT_LIB
)

option(MINT_DUMP_ASSEMBLY "Dump compiled assembly on debug" ON)
if (MINT_DUMP_ASSEMBLY)
	target_compile_definitions(
		mint-libmint
	PRIVATE
		-DMINT_DUMP_ASSEMBLY
	)
endif()

target_include_directories(
	mint-libmint
PUBLIC
	$<BUILD_INTERFACE:${MINT_INCLUDE_DIR}>
	$<INSTALL_INTERFACE:${MINT_HEADERS_INSTALL_DIR}>
PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}
)

target_sources(
	mint-libmint
PUBLIC
	FILE_SET ast TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES
	${MINT_INCLUDE_DIR}/mint/ast/abstract_syntax_tree_tools.h
	${MINT_INCLUDE_DIR}/mint/ast/abstract_syntax_tree_walker.h
	${MINT_INCLUDE_DIR}/mint/ast/abstract_syntax_tree.h
	${MINT_INCLUDE_DIR}/mint/ast/class_register.h
	${MINT_INCLUDE_DIR}/mint/ast/cursor.h
	${MINT_INCLUDE_DIR}/mint/ast/file_printer.h
	${MINT_INCLUDE_DIR}/mint/ast/module.h
	${MINT_INCLUDE_DIR}/mint/ast/node.h
	${MINT_INCLUDE_DIR}/mint/ast/printer.h
	${MINT_INCLUDE_DIR}/mint/ast/saved_state.h
	${MINT_INCLUDE_DIR}/mint/ast/symbol.h
PRIVATE
	ast/abstract_syntax_tree.cpp
	ast/abstract_syntax_tree_tools.cpp
	ast/class_register.cpp
	ast/cursor.cpp
	ast/file_printer.cpp
	ast/module.cpp
	ast/node.cpp
	ast/printer.cpp
	ast/saved_state.cpp
	ast/symbol.cpp
	ast/thread_entry_point.cpp
	ast/thread_entry_point.h
)

target_sources(
	mint-libmint
PUBLIC
	FILE_SET compiler TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES
	${MINT_INCLUDE_DIR}/mint/compiler/build_tools.h
	${MINT_INCLUDE_DIR}/mint/compiler/compiler.h
	${MINT_INCLUDE_DIR}/mint/compiler/lexical_handler.h
	${MINT_INCLUDE_DIR}/mint/compiler/lexer.h
	${MINT_INCLUDE_DIR}/mint/compiler/token.h
PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
	compiler/block.cpp
	compiler/block.h
	compiler/branch.cpp
	compiler/branch.h
	compiler/build_tools.cpp
	compiler/case_table.cpp
	compiler/case_table.h
	compiler/catch_context.cpp
	compiler/catch_context.h
	compiler/compiler.cpp
	compiler/context.cpp
	compiler/context.h
	compiler/lexer.cpp
	compiler/lexical_handler.cpp
	compiler/parser.y
)

target_sources(
	mint-libmint
PUBLIC
	FILE_SET debug TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES
	${MINT_INCLUDE_DIR}/mint/debug/cursor_debugger.h
	${MINT_INCLUDE_DIR}/mint/debug/debug_info.h
	${MINT_INCLUDE_DIR}/mint/debug/debug_interface.h
	${MINT_INCLUDE_DIR}/mint/debug/debug_tools.h
	${MINT_INCLUDE_DIR}/mint/debug/line_info.h
	${MINT_INCLUDE_DIR}/mint/debug/process_debugger.h
	${MINT_INCLUDE_DIR}/mint/debug/thread_context.h
PRIVATE
	debug/cursor_debugger.cpp
	debug/debug_info.cpp
	debug/debug_interface.cpp
	debug/debug_tools.cpp
	debug/line_info.cpp
	debug/process_debugger.cpp
)

target_sources(
	mint-libmint
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
	${MINT_INCLUDE_DIR}/mint/memory/algorithm.h
	${MINT_INCLUDE_DIR}/mint/memory/cast_tools.h
	${MINT_INCLUDE_DIR}/mint/memory/class.h
	${MINT_INCLUDE_DIR}/mint/memory/class_tools.h
	${MINT_INCLUDE_DIR}/mint/memory/data.h
	${MINT_INCLUDE_DIR}/mint/memory/function_tools.h
	${MINT_INCLUDE_DIR}/mint/memory/garbage_collector.h
	${MINT_INCLUDE_DIR}/mint/memory/global_data.h
	${MINT_INCLUDE_DIR}/mint/memory/memory_pool.h
	${MINT_INCLUDE_DIR}/mint/memory/memory_tools.h
	${MINT_INCLUDE_DIR}/mint/memory/object_printer.h
	${MINT_INCLUDE_DIR}/mint/memory/object.h
	${MINT_INCLUDE_DIR}/mint/memory/operator_tools.h
	${MINT_INCLUDE_DIR}/mint/memory/reference.h
	${MINT_INCLUDE_DIR}/mint/memory/symbol_table.h
PRIVATE
	memory/builtin/array.cpp
	memory/builtin/hash.cpp
	memory/builtin/iterator_async_generator.cpp
	memory/builtin/iterator_async_generator.h
	memory/builtin/iterator_generator.cpp
	memory/builtin/iterator_generator.h
	memory/builtin/iterator_items.cpp
	memory/builtin/iterator_items.h
	memory/builtin/iterator_range.cpp
	memory/builtin/iterator_range.h
	memory/builtin/iterator.cpp
	memory/builtin/libobject.cpp
	memory/builtin/library.cpp
	memory/builtin/regex.cpp
	memory/builtin/string.cpp
	memory/cast_tools.cpp
	memory/class.cpp
	memory/class_tools.cpp
	memory/data.cpp
	memory/function_tools.cpp
	memory/garbage_collector.cpp
	memory/global_data.cpp
	memory/memory_tools.cpp
	memory/object_printer.cpp
	memory/object.cpp
	memory/operator_tools.cpp
	memory/reference.cpp
	memory/symbol_table.cpp
)

target_sources(
	mint-libmint
PUBLIC
	FILE_SET scheduler TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES
	${MINT_INCLUDE_DIR}/mint/scheduler/destructor.h
	${MINT_INCLUDE_DIR}/mint/scheduler/exception.h
	${MINT_INCLUDE_DIR}/mint/scheduler/generator.h
	${MINT_INCLUDE_DIR}/mint/scheduler/input_stream.h
	${MINT_INCLUDE_DIR}/mint/scheduler/output.h
	${MINT_INCLUDE_DIR}/mint/scheduler/process.h
	${MINT_INCLUDE_DIR}/mint/scheduler/processor.h
	${MINT_INCLUDE_DIR}/mint/scheduler/scheduler.h
	${MINT_INCLUDE_DIR}/mint/scheduler/thread_pool.h
PRIVATE
	scheduler/brace_matcher.cpp
	scheduler/brace_matcher.h
	scheduler/completer.cpp
	scheduler/completer.h
	scheduler/destructor.cpp
	scheduler/exception.cpp
	scheduler/generator.cpp
	scheduler/highlighter.cpp
	scheduler/highlighter.h
	scheduler/input_stream.cpp
	scheduler/output.cpp
	scheduler/process.cpp
	scheduler/processor.cpp
	scheduler/scheduler.cpp
	scheduler/thread_pool.cpp
)

target_sources(
	mint-libmint
PUBLIC
	FILE_SET system TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES
	${MINT_INCLUDE_DIR}/mint/system/arguments.h
	${MINT_INCLUDE_DIR}/mint/system/assert.h
	${MINT_INCLUDE_DIR}/mint/system/buffer_stream.h
	${MINT_INCLUDE_DIR}/mint/system/data_stream.h
	${MINT_INCLUDE_DIR}/mint/system/errno.h
	${MINT_INCLUDE_DIR}/mint/system/error.h
	${MINT_INCLUDE_DIR}/mint/system/file_stream.h
	${MINT_INCLUDE_DIR}/mint/system/filesystem.h
	${MINT_INCLUDE_DIR}/mint/system/mint_runtime_error.h
	${MINT_INCLUDE_DIR}/mint/system/pipe.h
	${MINT_INCLUDE_DIR}/mint/system/plugin.h
	${MINT_INCLUDE_DIR}/mint/system/pool_allocator.h
	${MINT_INCLUDE_DIR}/mint/system/stdio.h
	${MINT_INCLUDE_DIR}/mint/system/string.h
	${MINT_INCLUDE_DIR}/mint/system/terminal.h
	${MINT_INCLUDE_DIR}/mint/system/utf8.h
PRIVATE
	system/arguments.cpp
	system/assert.cpp
	system/buffer_stream.cpp
	system/data_stream.cpp
	system/errno.cpp
	system/error.cpp
	system/file_stream.cpp
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
		mint-libmint
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
		mint-libmint
	PRIVATE
		system/unix/terminal.cpp
		system/unix/terminal.h
	)
endif()

target_sources(
	mint-libmint
PUBLIC
	FILE_SET config TYPE HEADERS
	BASE_DIRS ${MINT_INCLUDE_DIR}
	FILES ${MINT_INCLUDE_DIR}/mint/config.h
)

target_link_libraries(
	mint-libmint
PUBLIC
	Microsoft.GSL::GSL
PRIVATE
	ICU::i18n
	ICU::uc
	ICU::data
)

if (UNIX)
	target_link_libraries(
		mint-libmint
	PRIVATE
		pthread
		dl
	)
elseif (WIN32)
	target_link_libraries(
		mint-libmint
	PRIVATE
		userenv
	)
endif()

# Install
install(
	TARGETS mint-libmint
	EXPORT mint-libmint-targets
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
	EXPORT mint-libmint-targets
	FILE mint-libmint-targets.cmake
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
