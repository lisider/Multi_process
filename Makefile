all:process so process_test_so

process: process.c lib/* inc/*
	gcc process.c  thread_recv.c lib/*  -o main -pthread -g -I./inc -Wuninitialized

process_test_so:
	gcc process.c -o main2 -Iinc -lprocess -Lout -Wl,-rpath,out -pthread

so: lib/* inc/*
	[ -e out ] || mkdir out  
	gcc  thread_recv.c lib/*  -g -I./inc  -fPIC -shared -o out/libprocess.so -Wuninitialized
clean:
	rm main main2 out/* -rf
