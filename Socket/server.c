#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "socket.h"
#include "checksum.h"
#include "dynamic_buffer.h"
#include "server.h"

#define ERROR -1
#define OK 0
#define BYTES_SIZE 4
#define BLOCKS_TO_READ 1
#define ONE_BYTE 1
#define PREFIX_SIZE 1
#define CHKSUM_PREFIX_C '1'
#define EOF_CODE_C '2'
#define CHUNK_PREFIX_S '3'
#define BN_PREFIX_S '4'
#define EOF_CODE_S '5'
#define NOT_FOUND -1
#define STORE_SIZE 1000

typedef enum { BEGIN_READ, CHKS_FOUND, NO_CHKS_FOUND, NO_CHKS_END_READ } state;

int get_block_number(buffer_d_t *chksms, char sought_chksm[]){
	int checksum_block = NOT_FOUND;
	bool checksum_found = false;
	char checksum_byte;

	for (size_t i = 0; i < buffer_get_saved(chksms); i += BYTES_SIZE){
		if (checksum_block == NOT_FOUND){
			checksum_found = true;
			for (size_t j = 0; j < BYTES_SIZE; j++){
				buffer_get(chksms, i+j, &checksum_byte);
				checksum_found = (checksum_found) && (checksum_byte == sought_chksm[j]);
			}
			if (checksum_found){
				checksum_block = i / BYTES_SIZE;
			}
		}
	}

	return checksum_block;
}

int send_chunk(socket_t * socket, char chunk_to_send[], int chars_saved){
	int success = OK;

	char byte_chunk_prefix[PREFIX_SIZE] = { CHUNK_PREFIX_S };

	if (chars_saved > 0){
		//SEND CHARACTERS THAT DIDN'T MATCH CHECKSUMS:
		//FIRST SEND PREFIX 0x03:
		success += socket_send(socket, byte_chunk_prefix, PREFIX_SIZE);

		//SEND BYTE CHUNK LENGTH:
		char * length = ((char *)(&chars_saved));
		success += socket_send(socket, length, BYTES_SIZE);

		//SEND CHARACTERS:
		success += socket_send(socket, chunk_to_send, chars_saved);
	}

	return success;
}

int send_block_number(socket_t *socket, int block_number){
	int success = OK;
	char block_number_prefix[PREFIX_SIZE] = { BN_PREFIX_S };

	//SEND BLOCK NUMBER PREFIX 0x04:
	success = socket_send(socket, block_number_prefix, PREFIX_SIZE);

	if(success == OK){
		//Now send block number:
		char * bn = ((char *)(&block_number));
		success = socket_send(socket, bn, BYTES_SIZE);
	}

	return success;
}

int compare_file(socket_t *skt, FILE *file, buffer_d_t *chksms, int kBlockSize){
	int success = OK;
	int block_checksum = 0;
	int block_number = 0;
	int chars_saved = 0;
	int qty_final_read = 0;
	int blocks_read = 0;

	//INITIALIZE VARIABLE CONTAINING FILE SIZE
	//TO USE WHEN STORING BYTE CHUNK:
	fseek(file, 0, SEEK_END);
	int kFileSize = ftell(file);
	rewind(file);

    char current_block[kBlockSize+1];
    char eof_code[PREFIX_SIZE] = { EOF_CODE_S };
    char *chunk = malloc(sizeof(char)*(kFileSize+1));

	state current_state = BEGIN_READ;
	bool error_ocurred = false;
	while(!feof(file) && !error_ocurred){
		switch(current_state){
			case(BEGIN_READ):

				blocks_read = fread(current_block, kBlockSize, BLOCKS_TO_READ, file);
				block_checksum = calculate_checksum(current_block, kBlockSize);
				char * new_checksum = ((char *)(&block_checksum));
	    		block_number = get_block_number(chksms, new_checksum);

	    		error_ocurred = (blocks_read != BLOCKS_TO_READ);
	    		if(block_number == NOT_FOUND){
	    			if(ftell(file)>=kFileSize){
	    				current_state = NO_CHKS_END_READ;
	    			}else{
	    				current_state = NO_CHKS_FOUND;
	    			}
	    		}else{
	    			current_state = CHKS_FOUND;
	    		}

	    		break;

			case(NO_CHKS_FOUND):

				fseek(file, -kBlockSize, SEEK_CUR);
				blocks_read = fread(chunk + chars_saved, ONE_BYTE, BLOCKS_TO_READ, file);
				chars_saved += 1;
				current_state = BEGIN_READ;

				break;

			case(CHKS_FOUND):

				success += send_chunk(skt, chunk, chars_saved);
				success += send_block_number(skt, block_number);

				//ONCE CHARS+BN HAVE BEEN SENT GO BACK TO NORMAL READING MODE
				current_state = BEGIN_READ;
				chars_saved = 0;

				break;

			case(NO_CHKS_END_READ):

				//CALCULATE THE ACTUAL AMOUNT OF NON-EMPTY CHARACTERS READ:
				qty_final_read = kBlockSize - (ftell(file) - kFileSize);

				//RE-READ LAST BLOCK SO THAT ITS CHARACTERS ARE
				//INCLUDED IN char chars_saved[]:
				fseek(file, -kBlockSize, SEEK_CUR);
				blocks_read = fread(chunk+chars_saved, kBlockSize , BLOCKS_TO_READ, file);

				chars_saved += qty_final_read;
				success += send_chunk(skt, chunk, chars_saved);

				//GET TO END OF FILE:
				blocks_read = fread(current_block, kBlockSize , BLOCKS_TO_READ, file);

				break;
		}
	}

	free(chunk);

	if (feof(file) && (success == OK)){
		socket_send(skt, eof_code, PREFIX_SIZE);
		return OK;
	}

	return ERROR;
}

int save_checksums(socket_t *socket, buffer_d_t *chksms){
	char prefix_code[PREFIX_SIZE+1];
	char new_checksum[BYTES_SIZE+1];
	bool checksum_receive_complete = false;
	int success = OK;

	//RECEIVE CHECKSUMS FROM CLIENT;
	while ( (!checksum_receive_complete) && (success == OK) ){
		success = socket_receive(socket, prefix_code, PREFIX_SIZE);

		if(prefix_code[0] == EOF_CODE_C){
			checksum_receive_complete = true;
		}else if ((success == OK) && (prefix_code[0] == CHKSUM_PREFIX_C)){
			success = socket_receive(socket, new_checksum, BYTES_SIZE);
			bool saved = buffer_save(chksms, new_checksum, BYTES_SIZE);
			while (!saved){
				saved = buffer_save(chksms, new_checksum, BYTES_SIZE);
				buffer_resize(chksms);
			}
		}
	}
	if(success!=OK){
		return ERROR;
	}
	return OK;
}

bool server_state_is_valid(server_t *self){
	return (self->state == OK);
}

server_t *server_init(server_t *self, char hostname[], char port[]){
		self->state = OK;
		self->chksms = buffer_init(STORE_SIZE);
    self->state = socket_init(&self->aceptor, hostname, port, true);

    if (server_state_is_valid(self)){
	    self->state = socket_init(&self->listener, hostname, port, true);
    }

    if (server_state_is_valid(self)){
	    self->state = socket_bind_and_listen(&self->aceptor);
	    socket_accept(&self->aceptor, &self->listener);
    }
    return self;
}

void server_receive_file_name(server_t *self){
	if (!server_state_is_valid(self)){
    	return;
    }

    int *kNameSize;
    char file_name_size[BYTES_SIZE];

   	self->state = socket_receive(&self->listener, file_name_size, BYTES_SIZE);
   	kNameSize = (int*)(file_name_size);

   	self->file_name = calloc((*kNameSize + 1),sizeof(char));

   	if(server_state_is_valid(self)){
	   	self->state = socket_receive(&self->listener, self->file_name, *kNameSize);
	}
}

void server_save_chksms(server_t *self){
	if(!server_state_is_valid(self)){
		return;
	}

	char block_sz[BYTES_SIZE];

	self->state = socket_receive(&self->listener, block_sz, BYTES_SIZE);
	self->block_size = atoi(block_sz);

	if(server_state_is_valid(self)){
		self->state = save_checksums(&self->listener, self->chksms);
	}
}

void server_compare_and_send_chksms(server_t *self){
	if (!server_state_is_valid(self)){
		return;
	}

	FILE *local_file = NULL;
	local_file = fopen(self->file_name, "rb");

	if(local_file == NULL){
		self->state = ERROR;
	}

	if(server_state_is_valid(self)){
		self->state = compare_file(&self->listener,
									local_file,
									self->chksms,
									self->block_size);
	}

	if(local_file != NULL){
		fclose(local_file);
	}
}

void server_destroy(server_t *self){
	free(self->file_name);
	end_connection(&self->listener);
	end_connection(&self->aceptor);
	buffer_destroy(self->chksms);
}
