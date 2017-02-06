project (uart-test)
add_executable(uart-test src/main.c src/cmd.h src/cmd.c src/help.h src/help.c)
install(TARGETS uart-test DESTINATION bin)
