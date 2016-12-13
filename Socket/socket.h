#ifndef _SOCKET_H
#define _SOCKET_H

typedef struct socket socket_t;

struct socket{
	int fd;
	struct addrinfo *addr;
	char *hostname;
	char *port;
	bool set_flags;
};

int socket_init(socket_t *self, char hostname[], char port[], bool set_flags);

void socket_destroy(socket_t *self);

void socket_shutdown(socket_t *self);

int socket_send(socket_t *self, char *buffer, unsigned int size);

int socket_receive(socket_t *self, char *buffer, size_t size);

void socket_accept(socket_t *self, socket_t *connection_socket);

int socket_connect(socket_t *self);

int socket_bind_and_listen(socket_t *self);

void end_connection(socket_t *self);

#endif
