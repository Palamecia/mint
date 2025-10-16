project(mdbg)

find_package(Microsoft.GSL CONFIG REQUIRED)

# Target mdbg
add_executable(mdbg)

target_sources(
	mdbg
PRIVATE
	commandrunner.cpp
	commandrunner.h
	dapdebugger.cpp
	dapdebugger.h
	dapmessage.cpp
	dapmessage.h
	dapstream.cpp
	dapstream.h
	debugger.cpp
	debugger.h
	debuggerbackend.cpp
	debuggerbackend.h
	debugprinter.cpp
	debugprinter.h
	expressionevaluator.cpp
	expressionevaluator.h
	highlighter.cpp
	highlighter.h
	interactivedebugger.cpp
	interactivedebugger.h
	json.cpp
	json.h
	log.cpp
	log.h
	main.cpp
	stdstreampipe.cpp
	stdstreampipe.h
	symbolevaluator.cpp
	symbolevaluator.h
)

set_target_properties(
	mdbg
	PROPERTIES
	OUTPUT_NAME mdbg
	INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../lib64"
)

target_link_libraries(
	mdbg
PRIVATE 
	libmint
	Microsoft.GSL::GSL
)

# Install
install(
	TARGETS mdbg
	RUNTIME COMPONENT runtime DESTINATION ${MINT_RUNTIME_INSTALL_DIR}
)
