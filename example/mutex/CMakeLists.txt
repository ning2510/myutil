set(
    test_mutex
    ${PROJECT_SOURCE_DIR}/${PATH_EXAMPLE}/mutex/main.cc
)
add_executable(test_mutex ${test_mutex})
target_link_libraries(test_mutex ${LIBS})
install(TARGETS test_mutex DESTINATION ${PATH_BIN})