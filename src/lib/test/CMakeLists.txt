project(mint-test)

set(${PROJECT_NAME}_SRC)

set(${PROJECT_NAME}_LIB
	${MINT_LIBRARY_DIR}/test/case.mn
	${MINT_LIBRARY_DIR}/test/suite.mn
)

# add_library(${PROJECT_NAME}
# 	MODULE
# 	${${PROJECT_NAME}_SRC}
# 	${${PROJECT_NAME}_LIB}
# )

# target_link_libraries(${PROJECT_NAME} mint_library)
