project (uart-test)

add_executable(uart-test
	src/main.c
	src/cmd.h src/cmd.c
	src/help.h src/help.c
	src/sendbreak.c
	src/waitbreak.c
	src/ping.c
	src/iovec.c)

target_link_libraries(uart-test pthread)

install(TARGETS uart-test DESTINATION bin)
