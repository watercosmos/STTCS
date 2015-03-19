.PHNOY: all clean
all: main rxtx sink server arm-server

main: main.c
	@arm-linux-gnueabi-gcc -o main main.c -static
	@echo ... ./main
rxtx: rxtx.c utils.h
	@arm-linux-gnueabi-gcc -o rxtx rxtx.c -static -pthread
	@echo ... ./rxtx
sink: sink.c utils.h
	@arm-linux-gnueabi-gcc -o sink sink.c -static
	@echo ... ./sink
server: server.c
	@gcc -o server server.c
	@echo ... ./server
arm-server: server.c
	@arm-linux-gnueabi-gcc -o arm-server server.c -static
	@echo ... ./arm-server
clean:
	@rm main rxtx server sink arm-server recvfile
