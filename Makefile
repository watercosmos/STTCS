.PHNOY: all clean
# all: main rxtx sink server arm-server ./res/cgi httpd
all: main rxtx xmit

main: ./src/main.c
	@arm-linux-gnueabi-gcc -o main ./src/main.c -static
	@echo ... ./main
rxtx: ./src/rxtx.c ./src/utils.h
	@arm-linux-gnueabi-gcc -o rxtx ./src/rxtx.c -static -pthread
	@echo ... ./rxtx
xmit: ./src/xmit.c ./src/utils.h
	@arm-linux-gnueabi-gcc -o xmit ./src/xmit.c -static
	@echo ... ./xmit
# sink: ./src/sink.c ./src/utils.h
# 	@arm-linux-gnueabi-gcc -o sink ./src/sink.c -static
# 	@echo ... ./sink
# server: ./src/server.c
# 	@gcc -o server ./src/server.c
# 	@echo ... ./server
# arm-server: ./src/server.c
# 	@arm-linux-gnueabi-gcc -o arm-server ./src/server.c -static
# 	@echo ... ./arm-server
# httpd: ./src/httpd.c
# 	@arm-linux-gnueabi-gcc -o httpd ./src/httpd.c -static -pthread
# 	@echo ... ./httpd
# ./res/cgi: ./src/cgi.c
# 	@arm-linux-gnueabi-gcc -o ./res/cgi ./src/cgi.c -static
# 	@echo ... ./cgi

clean:
	@rm main rxtx xmit server sink arm-server ./res/cgi httpd recvfile
