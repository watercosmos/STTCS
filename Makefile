.PHNOY: all clean
all: main rxtx sink server arm-server cgi

main: ./src/main.c
	@arm-linux-gnueabi-gcc -o main ./src/main.c -static
	@echo ... ./main
rxtx: ./src/rxtx.c ./src/utils.h
	@arm-linux-gnueabi-gcc -o rxtx ./src/rxtx.c -static -pthread
	@echo ... ./rxtx
sink: ./src/sink.c ./src/utils.h
	@arm-linux-gnueabi-gcc -o sink ./src/sink.c -static
	@echo ... ./sink
server: ./src/server.c
	@gcc -o server ./src/server.c
	@echo ... ./server
arm-server: ./src/server.c
	@arm-linux-gnueabi-gcc -o arm-server ./src/server.c -static
	@echo ... ./arm-server
cgi: ./src/cgi.c
	@arm-linux-gnueabi-gcc -o cgi ./src/cgi.c -static
	@echo ... ./cgi

clean:
	@rm main rxtx server sink arm-server recvfile cgi
