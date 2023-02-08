project(mdbg_executable)

set(${PROJECT_NAME}_HEADERS
	dapdebugger.h
	dapmessage.h
	dapstream.h
	debugger.h
	debuggerbackend.h
	debugprinter.h
	highlighter.h
	interactivedebugger.h
	json.hpp
)

set(${PROJECT_NAME}_SOURCES
	dapdebugger.cpp
	dapmessage.cpp
	dapstream.cpp
	debugger.cpp
	debuggerbackend.cpp
	debugprinter.cpp
	highlighter.cpp
	interactivedebugger.cpp
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
install_executable(${PROJECT_NAME})
