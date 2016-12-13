#ifndef _SERVER_H
#define _SERVER_H

#include "dynamic_buffer.h"

typedef struct server server_t;

struct server{
	int state;
	FILE *file;
	socket_t aceptor;
	socket_t listener;
	buffer_d_t *chksms;
	int block_size;
	char *file_name;
};

server_t *server_init(server_t *self, char hostname[], char port[]);

bool server_state_is_valid(server_t *self);

void server_receive_file_name(server_t *self);

void server_save_chksms(server_t *self);

void server_compare_and_send_chksms(server_t *self);

void server_destroy(server_t *self);

#endif
