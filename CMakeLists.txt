project (uart-test)

add_executable(uart-test
	src/main.c
	src/cmd.h src/cmd.c
	src/help.h src/help.c
	src/sendbreak.c
	src/waitbreak.c
	src/ping.c
	src/buffer.c
	src/set_baud.c
	src/rts_control.c)

target_link_libraries(uart-test pthread)

install(TARGETS uart-test DESTINATION bin)
