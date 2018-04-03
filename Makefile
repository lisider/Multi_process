all:process

process: process.c
	gcc process.c  thread_recv.c lib/read_write_state_api.c  lib/shm.c lib/function.c  -o main -pthread -g -I./inc
clean:
	rm main -rf
