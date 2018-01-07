add_executable(tty-test tty-test.c)
target_link_libraries(tty-test ${LIBUV_LIBRARIES} -fsanitize=address -fsanitize=undefined)

add_executable(shell-test shell-test.c)
add_executable(printargs-test printargs-test.c)
