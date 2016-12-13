#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <math.h>
#include "socket.h"
#include "checksum.h"
#include "dynamic_buffer.h"
#include "client.h"

#define BYTES_SIZE 4
#define BLOCKS_TO_READ 1
#define BLOCKS_TO_WRITE 1
#define PREFIX_SIZE 1
#define CHKSUM_PREFIX_C '1'
#define EOF_CODE_C '2'
#define CHUNK_PREFIX_S '3'
#define BN_PREFIX_S '4'
#define EOF_CODE_S '5'
#define OK 0
#define ERROR 1

bool state_is_valid(client_t *self){
	return (self->state == OK); 
}

int send_server_file_name(client_t *self, char *file_name){
	int name_size = strlen(file_name);
	
	//Client sends updated file name size:
	char *name_length = (char*)(&name_size);
	self->state = socket_send(&self->socket, name_length, BYTES_SIZE);

	if(!state_is_valid(self)){
		return ERROR;	
	} 
	
	//Client sends updated file name:
	return socket_send(&self->socket, file_name, name_size);
}

int send_server_block_size(client_t *self){
	char block_size[BYTES_SIZE+1] = { *self->block_size };
	return socket_send(&self->socket, block_size, BYTES_SIZE);
}

int send_file_checksums(socket_t *socket, FILE *local_file, int kBlockSz){
    //Calculate checksum total amount:
	fseek(local_file, 0, SEEK_END);
	int file_size = ftell(local_file);
	int chksums_total_amount = file_size / kBlockSz;

    char current_block[kBlockSz+1];
    char eof_code[PREFIX_SIZE] = { EOF_CODE_C };
    char chksum_prefix[PREFIX_SIZE] = { CHKSUM_PREFIX_C };

	//Place pointer at beginning of file:
	rewind(local_file);

	//Read file and calculate checksums:
	int block_checksum;
	int blocks_read = 0;
	int total_checksums_read = 0;
	int success = OK;
	bool error_has_occurred = false;
	bool blocks_read_incomplete = true;
	bool total_chksms_not_reached = true;

    while ( total_chksms_not_reached && !error_has_occurred ){
		blocks_read = fread(current_block, kBlockSz , BLOCKS_TO_READ, local_file);

		blocks_read_incomplete = (blocks_read != BLOCKS_TO_READ);
		error_has_occurred = blocks_read_incomplete;
		total_chksms_not_reached = (total_checksums_read < chksums_total_amount);

		if (!error_has_occurred){
			success = socket_send(socket, chksum_prefix, PREFIX_SIZE);
			error_has_occurred = (error_has_occurred) || (success!=OK);
		}
		
		if (!error_has_occurred){
			block_checksum = calculate_checksum(current_block, kBlockSz);

			char * new_checksum = ((char *)(&block_checksum));

			success = socket_send(socket, new_checksum, BYTES_SIZE);

			total_checksums_read++;	

			error_has_occurred = (error_has_occurred) || (success!=OK);
		}
	}

	bool eof_reached = (total_checksums_read == chksums_total_amount);

	if(eof_reached){
		socket_send(socket, eof_code, PREFIX_SIZE);
		return OK;
	}
	
	return ERROR;
}

int update_file(socket_t *socket, FILE *new_file, FILE *old_file, int kBlockSz){
	char prefix_code[PREFIX_SIZE + 1];
	char old_block[kBlockSz+1];

	char chunk_size[BYTES_SIZE+1];
	int *kChunkLength;

	char block_number[BYTES_SIZE+1];
	int *block_pos;	
	
	int success = OK;
	int blocks_read = 0;
	
	//Receive checksums from client:
	bool info_receive_complete = false;

	while ( (!info_receive_complete) && (success == OK) ){
		success = socket_receive(socket, prefix_code, PREFIX_SIZE);

		if(prefix_code[0] == EOF_CODE_S){
			printf("RECV End of file\n");
			info_receive_complete = true;
		}else{ 
			if(prefix_code[0] == CHUNK_PREFIX_S){
				socket_receive(socket, chunk_size, BYTES_SIZE);
				kChunkLength = (int*)(chunk_size);
				
				char chunk[*kChunkLength];
				success = socket_receive(socket, chunk, *kChunkLength);

				printf("RECV File chunk %d bytes\n", (success == OK)? *kChunkLength : 0);

				fwrite(chunk, *kChunkLength ,BLOCKS_TO_WRITE, new_file);
			}else{
				if(prefix_code[0] == BN_PREFIX_S){
					success = socket_receive(socket, block_number, BYTES_SIZE);
					block_pos = (int*)(block_number);

					printf("RECV Block index %d\n", (success == OK)? *block_pos : 0);

					//Write block from old file in new file:
					fseek(old_file, (*block_pos)*kBlockSz, SEEK_SET);
					blocks_read = fread(old_block, kBlockSz, BLOCKS_TO_READ, old_file);
					fwrite(old_block, kBlockSz, BLOCKS_TO_WRITE, new_file);

					success = (blocks_read == BLOCKS_TO_READ)? OK : ERROR;
				}
			}
		}
	}
	if (success != OK){
		return ERROR;
	}
	return OK;
}

client_t *client_init(client_t *self, char hostname[], char port[]){
	self->state = OK;
	self->state = socket_init(&self->socket, hostname, port, false);
	if (state_is_valid(self)){
		self->state = socket_connect(&self->socket);
	}
	return self;
}

void client_send_fname(client_t *self, char *file_name, char *block_size){
	if (!state_is_valid(self)){
		return;
	}
	
	self->block_size = block_size;
	self->state = send_server_file_name(self, file_name);
	if (state_is_valid(self)){
		self->state = send_server_block_size(self);
	}
}

void client_send_chksms(client_t *self, char old_fname[], char new_fname[]){
	FILE *old_file = fopen(old_fname, "rb");
	FILE *new_file = fopen(new_fname, "wb");

	if ( (old_file == NULL) || (new_file == NULL) ){
		self->state = ERROR;
	}

	int block_size = atoi(self->block_size);

    if (state_is_valid(self)){
    	//Client sends file checksums to server:
   		self->state = send_file_checksums(&self->socket, old_file, block_size);
    }

    if (state_is_valid(self)){
    	update_file(&self->socket, new_file, old_file, block_size);	
    }

    if (old_file != NULL){
		fclose(old_file);
	}

	if (new_file != NULL){
		fclose(new_file);
	}
}

void client_destroy(client_t *self){
	end_connection(&self->socket);
}
