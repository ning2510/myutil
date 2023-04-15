set(
    test_coroutinePool
    ${PROJECT_SOURCE_DIR}/${PATH_EXAMPLE}/coroutinePool/main.cc
)
add_executable(test_coroutinePool ${test_coroutinePool})
target_link_libraries(test_coroutinePool ${LIBS})
install(TARGETS test_coroutinePool DESTINATION ${PATH_BIN})