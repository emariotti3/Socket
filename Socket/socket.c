#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "socket.h"

#define MAXLEN_CONNECTION_QUEUE 10
#define ACCEPT_ERROR 1
#define CONNECT_ERROR 1
#define INVALID_DESC 1
#define ERROR 1
#define OK 0

int socket_init(socket_t *self, char hostname[], char port[], bool set_flags){
		struct addrinfo hints;

		memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = set_flags? AI_PASSIVE:0;

		getaddrinfo(hostname, port, &hints, &self->addr);
		int new_fd = socket(self->addr->ai_family,
							self->addr->ai_socktype,
							self->addr->ai_protocol);

		if (new_fd == INVALID_DESC){
			return ERROR;
		}
		self->hostname = hostname;
		self->port = port;
		self->fd = new_fd;
		self->set_flags = set_flags;
		return OK;
}

void socket_destroy(socket_t *self){
		freeaddrinfo(self->addr);
		close(self->fd);
}

void socket_shutdown(socket_t *self){
		shutdown(self->fd, SHUT_RDWR);
}

int socket_bind_and_listen(socket_t *self){
		int success = bind(self->fd,
							self->addr->ai_addr,
							self->addr->ai_addrlen);

		if (success != OK){
			return ERROR;
		}

		int listen_successful = listen(self -> fd, MAXLEN_CONNECTION_QUEUE);

		if(listen_successful != OK){
			return ERROR;
		}

		return OK;
}

void socket_accept(socket_t *self, socket_t *connection_socket){
		bool continue_running = true;
		int new_fd;

		while (continue_running){
			new_fd = accept(self -> fd, NULL, NULL);

			if (new_fd != ACCEPT_ERROR){
				connection_socket -> fd = new_fd;
				continue_running = false;
			}
		}
}

int socket_connect(socket_t *self){
		int success = connect(self->fd,
								self->addr->ai_addr,
								self->addr->ai_addrlen);

		bool connection_list_ended = (self->addr == NULL);
		bool fd_valid = false;
		int fd = 0;
		while ( (!connection_list_ended) && (success == CONNECT_ERROR) ){
			self->addr = self->addr-> ai_next;
			connection_list_ended = (self->addr == NULL);

			fd = socket_init(self, self->port, self->hostname, self->set_flags);
			fd_valid = (fd != INVALID_DESC);

			if ( (!connection_list_ended) && fd_valid ){
				success = connect(self->fd,
									self->addr->ai_addr,
									self->addr->ai_addrlen);
			}
		}

		if (success == CONNECT_ERROR){
			return ERROR;
		}

		return OK;
}

int socket_send(socket_t *self, char *buffer, unsigned int size){
		int sent = 0;
		int total_sent = 0;

		while ((size - total_sent) != 0){
			sent = send(self -> fd, buffer+total_sent, size - total_sent, MSG_NOSIGNAL);

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
		char *pos = NULL;

		while ((size - total_received) != 0){
			pos = buffer + total_received;
			read = recv(self -> fd, pos, size - total_received, MSG_NOSIGNAL);

			if (read < 0){
				return ERROR;
			}

			total_received += read;
		}

		return OK;
}

void end_connection(socket_t *self){
		socket_shutdown(self);
		socket_destroy(self);
}
