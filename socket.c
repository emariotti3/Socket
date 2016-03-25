#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "socket.h"

#define MAXLEN_CONNECTION_QUEUE 10
#define ACCEPT_ERROR -1
#define CONNECT_ERROR -1
#define INVALID_DESCRIPTOR -1
#define ERROR -1
#define OK 0

int socket_init(socket_t *self, struct addrinfo *addr_info){

	int new_file_descriptor = socket(addr_info -> ai_family, addr_info -> ai_socktype, addr_info -> ai_protocol);
	
	if (new_file_descriptor == INVALID_DESCRIPTOR){
		return ERROR;
	}

	self->file_descriptor = new_file_descriptor;
	return OK;
}

void socket_destroy(socket_t *self){

	close(self -> file_descriptor);
}

void socket_shutdown(socket_t *self){

	shutdown(self -> file_descriptor, SHUT_RDWR);
}

int socket_bind_and_listen(socket_t *self, struct addrinfo *addr_info){

	int bind_successful = bind(self -> file_descriptor, addr_info -> ai_addr, addr_info -> ai_addrlen);

	if (bind_successful != OK){
		return ERROR;	
	}

	int listen_successful = listen(self -> file_descriptor, MAXLEN_CONNECTION_QUEUE);

	if(listen_successful != OK){
		return ERROR;	
	}

	return OK;
}

void socket_accept(socket_t *self, socket_t *connection_socket){

	bool continue_running = true;
	int new_file_descriptor;

	while (continue_running){

		new_file_descriptor = accept(self -> file_descriptor, NULL, NULL);

		if (new_file_descriptor != ACCEPT_ERROR){
			connection_socket -> file_descriptor = new_file_descriptor;
			continue_running = false;
		}
	}

}

int socket_connect(socket_t *server, struct addrinfo *addr_info){

	int connect_successful = connect(server -> file_descriptor, addr_info -> ai_addr, addr_info -> ai_addrlen);
	bool connection_list_ended = (addr_info == NULL);

	while ( (!connection_list_ended) && (connect_successful == CONNECT_ERROR) ){

		addr_info = addr_info -> ai_next;
		connection_list_ended = (addr_info == NULL); 

		if ((!connection_list_ended) && (socket_init(server, addr_info) != INVALID_DESCRIPTOR)){
			connect_successful = connect(server -> file_descriptor, addr_info -> ai_addr, addr_info -> ai_addrlen);	
		}		
	}

	if (connect_successful == CONNECT_ERROR){
		return ERROR;
	}

	return OK;
}

int socket_send(socket_t *self, char *buffer, unsigned int size){

	int sent = 0;
	int total_sent = 0;

	while ((size - total_sent) != 0){
		sent = send(self -> file_descriptor, buffer + total_sent, size - total_sent, MSG_NOSIGNAL);

		if (sent < 0){
			return ERROR;
		}

		total_sent += sent;
	}

	return OK;
}

int socket_receive(socket_t *self, char *buffer, size_t size){

	int read = 0;
	int total_received = 0;

	while ((size - total_received) != 0){
		read = recv(self -> file_descriptor, buffer + total_received, size - total_received, MSG_NOSIGNAL);

		if (read < 0){
			return ERROR;
		}

		total_received += read;
	}
	
	return OK;
}
