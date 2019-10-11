all:	main.c server.o config_manager.o
	gcc main.c server.o config_manager.o -g -o fish -lpthread -lasound

server.o: server.c
	gcc -c server.c	-g 

config_manager.o: config_manager.c
	gcc -c config_manager.c -g 
