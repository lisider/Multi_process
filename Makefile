all:process

process: process.c
	gcc process.c  read_write_state_api.c  shm.c function.c thread_recv.c -o main -pthread -g
clean:
	rm main -rf
