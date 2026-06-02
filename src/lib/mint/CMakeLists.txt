project(mint-mint)

add_library(mint-lib-module-mint MODULE)

set_target_properties(
	mint-lib-module-mint
	PROPERTIES
	PREFIX lib
	OUTPUT_NAME mint-mint
	INSTALL_RPATH "\$ORIGIN:\$ORIGIN/.."
)

target_sources(
	mint-lib-module-mint
PRIVATE
	assembly.cpp
	coroutine.cpp
	eval_result_printer.cpp
	eval_result_printer.h
	function.cpp
	garbage_collector.cpp
	int_types.cpp
	lang.cpp
	lexical_handler.cpp
	math.cpp
	operator.cpp
	printer.cpp
	stacktrace.cpp
	string.cpp
	type.cpp
)

target_link_libraries(
	mint-lib-module-mint
PRIVATE
	mint::mint
)

# Install
install(
	TARGETS mint-lib-module-mint
	LIBRARY COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	ARCHIVE COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
	RUNTIME COMPONENT runtime DESTINATION ${MINT_LIBRARY_INSTALL_DIR}/mint
)

add_dependencies(mint-lib-module-mint mint-module-mint)
add_dependencies(mint-lib-module-mint mint-module-container)
