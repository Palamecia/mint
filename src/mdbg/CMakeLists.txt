project(mdbg_executable)

# Target mdbg_executable
add_executable(${PROJECT_NAME})

target_sources(
	${PROJECT_NAME}
	PRIVATE
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
	json.hpp
	main.cpp
	stdstreampipe.cpp
	stdstreampipe.h
	symbolevaluator.cpp
	symbolevaluator.h
)

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME mdbg
	INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../lib64"
)

target_link_libraries(
	${PROJECT_NAME}
	mint_library
)

# Install
mint_install_executable(${PROJECT_NAME})
