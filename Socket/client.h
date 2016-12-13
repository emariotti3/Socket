#ifndef _CLIENT_H
#define _CLIENT_H

typedef struct client client_t;

struct client{
	int state;
	char *block_size;
	socket_t socket;
};

client_t *client_init(client_t *self, char *hostname, char *port);

void client_send_fname(client_t *self, char *file_name, char *block_size);

void client_send_chksms(client_t *self, char old_fname[], char new_fname[]);

bool state_is_valid(client_t *self);

void client_destroy(client_t *self);

#endif
