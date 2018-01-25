project(mint-test)

set(${PROJECT_NAME}_SRC)

set(${PROJECT_NAME}_LIB
	${MINT_MODULES_DIR}/test/case.mn
	${MINT_MODULES_DIR}/test/suite.mn
)

# add_library(${PROJECT_NAME}
# 	MODULE
# 	${${PROJECT_NAME}_SRC}
# )

# target_link_libraries(${PROJECT_NAME} mint_library)

# Install
# install_library(${PROJECT_NAME} SUBDIR mint)
install_modules(${${PROJECT_NAME}_LIB} SUBDIR test)
