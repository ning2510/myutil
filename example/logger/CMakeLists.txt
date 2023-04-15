set(
    test_logger
    ${PROJECT_SOURCE_DIR}/${PATH_EXAMPLE}/logger/main.cc
)
add_executable(test_logger ${test_logger})
target_link_libraries(test_logger ${LIBS})
install(TARGETS test_logger DESTINATION ${PATH_BIN})