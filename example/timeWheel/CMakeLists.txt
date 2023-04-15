set(
    test_timeWheel_server
    ${PROJECT_SOURCE_DIR}/${PATH_EXAMPLE}/timeWheel/server.cc
)
add_executable(test_timeWheel_server ${test_timeWheel_server})
target_link_libraries(test_timeWheel_server ${LIBS})
install(TARGETS test_timeWheel_server DESTINATION ${PATH_BIN})

set(
    test_timeWheel_client
    ${PROJECT_SOURCE_DIR}/${PATH_EXAMPLE}/timeWheel/client.cc
)
add_executable(test_timeWheel_client ${test_timeWheel_client})
target_link_libraries(test_timeWheel_client pthread)
install(TARGETS test_timeWheel_client DESTINATION ${PATH_BIN})