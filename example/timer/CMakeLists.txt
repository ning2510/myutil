set(
    test_timer
    ${PROJECT_SOURCE_DIR}/${PATH_EXAMPLE}/timer/main.cc
)
add_executable(test_timer ${test_timer})
target_link_libraries(test_timer ${LIBS})
install(TARGETS test_timer DESTINATION ${PATH_BIN})