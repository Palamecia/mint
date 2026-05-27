project(mdbg)

find_package(Microsoft.GSL CONFIG REQUIRED)

# Target mdbg
add_executable(mint-mdbg)

set_target_properties(
	mint-mdbg
	PROPERTIES
	OUTPUT_NAME mdbg
	INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../lib64"
)

target_sources(
	mint-mdbg
PRIVATE
	command_runner.cpp
	command_runner.h
	dap_debugger.cpp
	dap_debugger.h
	dap_message.cpp
	dap_message.h
	dap_stream.cpp
	dap_stream.h
	debug_printer.cpp
	debug_printer.h
	debugger_backend.cpp
	debugger_backend.h
	debugger.cpp
	debugger.h
	expression_evaluator.cpp
	expression_evaluator.h
	highlighter.cpp
	highlighter.h
	interactive_debugger.cpp
	interactive_debugger.h
	json.cpp
	json.h
	log.cpp
	log.h
	main.cpp
	std_stream_pipe.cpp
	std_stream_pipe.h
	symbol_evaluator.cpp
	symbol_evaluator.h
)

target_link_libraries(
	mint-mdbg
PRIVATE 
	mint::mint
	Microsoft.GSL::GSL
)

# Install
install(
	TARGETS mint-mdbg
	RUNTIME COMPONENT runtime DESTINATION ${MINT_RUNTIME_INSTALL_DIR}
)
