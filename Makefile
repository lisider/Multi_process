all:process so

process: process.c lib/* inc/*
	gcc process.c  thread_recv.c lib/*  -o main -pthread -g -I./inc -Wuninitialized
so: lib/* inc/*
	[ -e out ] || mkdir out  
	gcc  thread_recv.c lib/*  -g -I./inc  -fPIC -shared -o out/libprocess.so -Wuninitialized
clean:
	rm main out/* -rf
