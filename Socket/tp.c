#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "socket.h"
#include "checksum.h"
#include "client.h"
#include "dynamic_buffer.h"
#include "server.h"

#define SYSTEM_EXIT 0
#define POS_OLD_FILE_NAME 4
#define POS_NEW_FILE_NAME 5
#define POS_PORT_S 2
#define POS_PORT_C 3
#define POS_MODE 1
#define POS_SERVER_FILE_NAME 6
#define POS_BLOCK_SIZE 7
#define POS_HOSTNAME_C 2
#define OK 0
#define PARAMETER_ERROR 1
#define BYTES_SIZE 4
#define PARAMETERS_MIN 2
#define PARAMETERS_MAX 8
#define MODE_LENGTH 7
#define CLIENT_MODE "client"
#define SERVER_MODE "server"


bool parameters_are_valid(int argc, char *argv[]){
	bool parameter_count_ok = (argc <= PARAMETERS_MAX) && (argc >= PARAMETERS_MIN);

	if( !parameter_count_ok ){
		return false;
	}
	return true;
}

int main(int argc, char *argv[]){
	bool parameters_valid = parameters_are_valid(argc, argv);
	char client_mode[MODE_LENGTH+1] = CLIENT_MODE;
	char server_mode[MODE_LENGTH+1] = SERVER_MODE;

	bool mode_is_client = (strcmp(argv[POS_MODE], client_mode) == OK);
	bool mode_is_server = (strcmp(argv[POS_MODE], server_mode) == OK);

	if (mode_is_client && parameters_valid){
		client_t client;
		client_init(&client, argv[POS_HOSTNAME_C], argv[POS_PORT_C]);
		client_send_fname(&client, argv[POS_SERVER_FILE_NAME], argv[POS_BLOCK_SIZE]);
		client_send_chksms(&client, argv[POS_OLD_FILE_NAME], argv[POS_NEW_FILE_NAME]);
		client_destroy(&client);
		return SYSTEM_EXIT;
	}else if (mode_is_server && parameters_valid){
		server_t server;
		server_init(&server, NULL, argv[POS_PORT_S]);
		server_receive_file_name(&server);
		server_save_chksms(&server);
		server_compare_and_send_chksms(&server);
		server_destroy(&server);
		return SYSTEM_EXIT;
	}
	return PARAMETER_ERROR;
}

