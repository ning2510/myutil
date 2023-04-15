set(
    test_http_server
    ${PROJECT_SOURCE_DIR}/${PATH_EXAMPLE}/http/server.cc
)
add_executable(test_http_server ${test_http_server})
target_link_libraries(test_http_server ${LIBS})
install(TARGETS test_http_server DESTINATION ${PATH_BIN})