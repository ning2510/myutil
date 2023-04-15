set(
    test_buffer
    ${PROJECT_SOURCE_DIR}/${PATH_EXAMPLE}/buffer/main.cc
)
add_executable(test_buffer ${test_buffer})
target_link_libraries(test_buffer ${LIBS})
install(TARGETS test_buffer DESTINATION ${PATH_BIN})