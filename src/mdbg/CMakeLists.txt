project(mdbg_executable)

set(${PROJECT_NAME}_HEADERS
	dapdebugger.h
	dapmessage.h
	dapstream.h
	debugger.h
	debuggerbackend.h
	debugprinter.h
	expressionevaluator.h
	highlighter.h
	interactivedebugger.h
	json.hpp
	stdstreampipe.h
	symbolevaluator.h
)

set(${PROJECT_NAME}_SOURCES
	dapdebugger.cpp
	dapmessage.cpp
	dapstream.cpp
	debugger.cpp
	debuggerbackend.cpp
	debugprinter.cpp
	expressionevaluator.cpp
	highlighter.cpp
	interactivedebugger.cpp
	stdstreampipe.cpp
	symbolevaluator.cpp
)

# Target mint_executable
add_executable(
	${PROJECT_NAME}
	${${PROJECT_NAME}_HEADERS}
	${${PROJECT_NAME}_SOURCES}
	main.cpp
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
