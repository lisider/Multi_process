all:process so

process: process.c lib/* inc/*
	#	gcc process.c  thread_recv.c lib/read_write_state_api.c  lib/shm.c lib/function.c  -o main -pthread -g -I./inc
	gcc process.c  thread_recv.c lib/*  -o main -pthread -g -I./inc
so: lib/* inc/*
	[ -e out ] || mkdir out  
	gcc  thread_recv.c lib/*  -g -I./inc  -fPIC -shared -o out/libprocess.so
clean:
	rm main out/* -rf
