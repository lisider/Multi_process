all:process

process: process_sample.c
	gcc process_sample.c  read_write_state_api.c -o process -pthread
clean:
	rm process
