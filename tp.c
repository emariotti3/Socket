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

#define POS_MODE 1
#define POS_PORT_C 3
#define POS_PORT_S 2
#define POS_HOSTNAME_C 2
#define POS_OLD_FILE_NAME 4
#define POS_NEW_FILE_NAME 5
#define POS_SERVER_FILE_NAME 6
#define POS_BLOCK_SIZE 7
#define SYSTEM_EXIT 0
#define ERROR -1
#define OK 0
#define PARAMETER_ERROR 1
#define BYTES_SIZE 4
#define NUMERIC_BASE 10
#define BLOCKS_TO_READ 1
#define PREFIX_SIZE 1
#define CHKSUM_PREFIX_C '1'
#define EOF_CODE_C '2'
#define CHUNK_PREFIX_S '3'
#define BN_PREFIX_S '4'
#define EOF_CODE_S '5'
#define SIZE_CHKS_STORE 1000
#define NOT_FOUND -1

typedef enum { BEGIN_READ, CHKS_FOUND, NO_CHKS_FOUND } state;

/*******FUNCIONES DE USO COMUN*****************/

int calculate_checksum(char block[], int block_size){

	int lower = 0;
	int higher = 0;
	int ascii = 0;
	long m = pow(2,16);

	for (int i = 0; i < block_size; i++){
		ascii = (int) block[i];
		lower += ascii;
		higher += ascii * (block_size - i);
	}
	lower = lower % m;
	higher = higher % m;
	return lower + (higher * m);
	
}

void end_connection(socket_t *socket){

	socket_shutdown(socket);
	socket_destroy(socket);

}

/****************************************************/

/***************CLIENTE*****************************/

int send_server_file_name(socket_t *socket, char *argv[]){

	size_t name_size = strlen(argv[POS_SERVER_FILE_NAME]);
	
	//CLIENT SENDS UPDATED FILE NAME SIZE:
	char name_length[BYTES_SIZE];
	snprintf(name_length,BYTES_SIZE,"%d",(int)name_size);
	
	int success = socket_send(socket, name_length, BYTES_SIZE);

	if(success != OK){
		return ERROR;	
	} 

	//CLIENT SENDS UPDATED FILE NAME:
	return socket_send(socket, argv[POS_SERVER_FILE_NAME], name_size);

}

int send_server_block_size(socket_t *socket, char *argv[]){

	char block_size[BYTES_SIZE] = { *argv[POS_BLOCK_SIZE] };
	return socket_send(socket, block_size, BYTES_SIZE);

}

int send_server_file_checksums(socket_t *socket, char *file, char *block_sz){

	int block_size = atoi(block_sz);
	FILE *outdated_local_file = fopen(file,"rb");

    //CALCULATE CHECKSUM TOTAL AMOUNT:
	fseek(outdated_local_file, 0, SEEK_END);
	int file_size = ftell(outdated_local_file);
	int chksums_total_amount = file_size / block_size;

    char current_block[block_size];
    char eof_code[PREFIX_SIZE] = { EOF_CODE_C };
    char chksum_prefix[PREFIX_SIZE] = { CHKSUM_PREFIX_C };

	//PLACE POINTER AT BEGINNING OF FILE:
	rewind(outdated_local_file);

	//READ FILE AND CALCULATE CHECKSUMS:
	int block_checksum;
	int blocks_read = 0;
	int total_checksums_read = 0;
	int success = OK;
	bool error_has_occurred = false;

    while( (total_checksums_read < chksums_total_amount) && !error_has_occurred ){

		blocks_read = fread(current_block, block_size , BLOCKS_TO_READ, outdated_local_file);

		error_has_occurred = ( blocks_read != BLOCKS_TO_READ ) && (total_checksums_read < chksums_total_amount);

		if(!error_has_occurred){

			success = socket_send(socket, chksum_prefix, PREFIX_SIZE);
			error_has_occurred = (error_has_occurred) || (success!=OK);

		}
		
		if(!error_has_occurred){

			block_checksum = calculate_checksum(current_block, block_size);

			char * new_checksum = ((char *)(&block_checksum));

			success = socket_send(socket, new_checksum, BYTES_SIZE);

			total_checksums_read++;	

			error_has_occurred = (error_has_occurred) || (success!=OK);

		}
					
	}

	fclose(outdated_local_file);
	bool eof_reached = (total_checksums_read == chksums_total_amount);

	if(eof_reached){
		socket_send(socket, eof_code, PREFIX_SIZE);
		return OK;
	}
	
	return ERROR;
 }

int receive_info_and_create_file(socket_t *socket, FILE *new_file){

	char prefix_code[PREFIX_SIZE];

	char chunk_size[BYTES_SIZE];
	char chunk[BYTES_SIZE];
	int chunk_length;

	char block_number[BYTES_SIZE];
	int block;	
	
	int success = OK;
	bool info_receive_complete = false;
	
	//RECEIVE CHECKSUMS FROM CLIENT;
	while ( (!info_receive_complete) && (success == OK) ){

		success = socket_receive(socket, prefix_code, PREFIX_SIZE);

		switch(prefix_code[0]){

			case(EOF_CODE_S):

				printf("RECV End of file \n");
				info_receive_complete = true;

			case(CHUNK_PREFIX_S):

				success = socket_receive(socket, chunk_size, BYTES_SIZE);
				chunk_length = atoi(chunk_size);
				printf("RECV File chunk %d bytes \n", chunk_length);
				socket_receive(socket, chunk, chunk_length);

			case(BN_PREFIX_S):

				success = socket_receive(socket, block_number, BYTES_SIZE);
				block = atoi(block_number);
				printf("RECV Block index %d \n", block);

		}

	}

	if(success!=OK){
		return ERROR;
	}
	
	return OK;

}

int trigger_client_mode(char *argv[]){

	struct addrinfo hints;
	struct addrinfo *result;
	FILE *new_file;

	socket_t client;

	int success = 0;

	//***********INITIALIZE CLIENT***********//

	memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;       /* IPv4 */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_flags = 0;

    success = getaddrinfo(argv[POS_HOSTNAME_C], argv[POS_PORT_C], &hints, &result);

    if (success == OK){

   		success = socket_init(&client, result);

    }

    if (success == OK){

   		success = socket_connect(&client,result);

    }

    /**************************************/
    
    if (success == OK){
    	//Client sends information needed for server
    	//to retrieve updated file:
   		success = send_server_file_name(&client, argv);

    }

    if (success == OK){
	    //Client sends block size to server:
   		success = send_server_block_size(&client, argv);

    }

    if (success == OK){
    	//Client sends file checksums to server:
   		success = send_server_file_checksums(&client, argv[POS_OLD_FILE_NAME], argv[POS_BLOCK_SIZE]);

    }

    if (success == OK){

    	new_file = fopen(argv[POS_NEW_FILE_NAME], "rb");
    	receive_info_and_create_file(&client, new_file);
    	fclose(new_file);
    }

	//TODO: RECEIVE AND WRITE NEW FILE!

	end_connection(&client);

	return SYSTEM_EXIT;
}

/****************************************************/

/****************SERVER******************************/

int get_checksum_block_number(char checksums[], char sought_chksm[], int chksm_size){

	int checksum_block = NOT_FOUND;
	bool checksum_found;
	
	for ( int i = 0; checksums[i]; i += chksm_size){

		if (checksum_block == NOT_FOUND){

			checksum_found = true;
			for (int j = 0; j < chksm_size; j++){

				checksum_found = (checksum_found) && (checksums[j] == sought_chksm[j]);

			}
			if (checksum_found){
				checksum_block = i;
			}
		}
	}

	return checksum_block;

}

int send_chunk(socket_t * socket, FILE *local_file, int chars_saved, int file_size){

	int success = OK;
	char chars_to_send[file_size];
	char byte_chunk_prefix[PREFIX_SIZE] = { CHUNK_PREFIX_S };

	if (chars_saved > 0){

		//READ AL THE CHARS THAT NEED TO BE SENT TO CLIENT 
		//AND STORE THEM IN char chars_to_send[]:
		fseek(local_file, -(ftell(local_file) - chars_saved), SEEK_CUR);

		success = fread(chars_to_send, chars_saved, BLOCKS_TO_READ, local_file);

		//SEND CHARACTERS THAT DIDN'T MATCH CHECKSUMS:
		//FIRST SEND PREFIX 0x03:
		socket_send(socket, byte_chunk_prefix, PREFIX_SIZE);

		//SEND BYTE CHUNK LENGTH:
		char * length = ((char *)(&chars_saved));
		socket_send(socket, length, BYTES_SIZE);

		//SEND CHARACTERS:
		socket_send(socket, chars_to_send, chars_saved);
	}

	return success;
}

int send_block_number(socket_t *socket, int block_number){

	int success = OK;
	char block_number_prefix[PREFIX_SIZE] = { BN_PREFIX_S };

	//SEND BLOCK NUMBER PREFIX 0x04:
	success = socket_send(socket, block_number_prefix, PREFIX_SIZE);

	if(success == OK){

		//NOW SEND BLOCK NUMBER:
		char * bn = ((char *)(&block_number));
		success = socket_send(socket, bn, BYTES_SIZE);
	}

	return success;

}

int compare_local_file(socket_t *socket, FILE *local_file, char received_checksums[], int block_size, int chksm_size){

	int success = OK;
    char current_block[block_size];
    char eof_code[PREFIX_SIZE] = { EOF_CODE_S };
    

	int block_checksum;
	int block_number;
	int blocks_read;
	int chars_saved = 0;

	//INITIALIZE VARIABLE CONTAINING FILE SIZE
	//TO USE WHEN STORING BYTE CHUNK:
	fseek(local_file, 0, SEEK_END);
	int file_size = ftell(local_file);
	rewind(local_file);

	state current_state = BEGIN_READ;
	bool eof_reached = (ftell(local_file) > file_size);

	while(!eof_reached){

		switch(current_state){

			case(BEGIN_READ):

				blocks_read = fread(current_block, block_size , BLOCKS_TO_READ, local_file);

				block_checksum = calculate_checksum(current_block, block_size);

				char * new_checksum = ((char *)(&block_checksum));

	    		block_number = get_checksum_block_number(received_checksums, new_checksum, chksm_size);

	    		if(block_number == NOT_FOUND){

	    			current_state = NO_CHKS_FOUND;

	    		}else{

	    			current_state = CHKS_FOUND;

	    		}

	    		break;

			case(NO_CHKS_FOUND):

				//GO BACK TO BEGINNING OF BLOCK ADD +1 TO THE AMOUNT OF CHARS
				//THAT NEED TO BE SENT:

				success = fseek(local_file, ftell(local_file)-(block_size)+1, SEEK_CUR);

				chars_saved += 1;

				break;			

			case(CHKS_FOUND):
				
				send_chunk(socket, local_file, chars_saved, file_size);

				send_block_number(socket, block_number);

				//ONCE CHARS+BN HAVE BEEN SENT GO BACK TO THE BLOCK WHERE CHECKSUM WAS FOUND:
				current_state = BEGIN_READ;

				chars_saved = 0;

		}

	    eof_reached = (ftell(local_file) > file_size);

	}

	if(eof_reached){

		send_chunk(socket, local_file, chars_saved, file_size);

		socket_send(socket, eof_code, PREFIX_SIZE);

		return OK;

	}
	
	return ERROR;


}

int receive_checksums(socket_t *socket, char *checksums_received, int chks_size, int size_chks_store){

	char prefix_code[PREFIX_SIZE];
	char new_checksum[chks_size];
	bool checksum_receive_complete = false;
	int total_checksums_received = 0;
	int success = OK;
	

	//RECEIVE CHECKSUMS FROM CLIENT;
	while ( (!checksum_receive_complete) && (success == OK) ){

		success = socket_receive(socket, prefix_code, PREFIX_SIZE);

		if(prefix_code[0] == EOF_CODE_C){

			checksum_receive_complete = true;

		}else if (success == OK){

			if ( total_checksums_received == ( size_chks_store / 2 )){

				checksums_received = realloc(checksums_received, size_chks_store);

			}

			success = socket_receive(socket, new_checksum, chks_size);

			for(int i = 0; i < chks_size; i++){
				
				checksums_received[i + total_checksums_received] = new_checksum[i];
			}

			total_checksums_received += chks_size;

		}

	}

	if(success!=OK){
		return ERROR;
	}
	
	return OK;

}

int trigger_server_mode(char *argv[]){

	int success = 0;
	int block_size = 0;
	struct addrinfo hints;
	struct addrinfo *ptr;
	FILE *updated_local_file;
	char file_name_size[BYTES_SIZE];
	char *checksums_received = malloc(SIZE_CHKS_STORE * sizeof(char));

	socket_t aceptor;
	socket_t listener;

	//***********INITIALIZE SERVER***********//

	memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;       /* IPv4 */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_flags = AI_PASSIVE;

    success = getaddrinfo(NULL, argv[POS_PORT_S], &hints, &ptr);

    if (success == OK){

    	success = socket_init(&aceptor, ptr);

    }

    //***************************************//

    if (success == OK){

    	success = socket_bind_and_listen(&aceptor,ptr);

    }

    if (success == OK){

    	socket_accept(&aceptor,&listener);
   		success = socket_receive(&listener, file_name_size, BYTES_SIZE);
   			
   	}

   	int name_length = atoi(file_name_size);
	char file_name[name_length];

	if(success == OK){

		success = socket_receive(&listener, file_name, name_length);

	}

	char block_sz[BYTES_SIZE];

	if(success == OK){
		
		success = socket_receive(&listener, block_sz, BYTES_SIZE);

	}

	if(success == OK){

		success = receive_checksums(&listener, checksums_received, BYTES_SIZE, SIZE_CHKS_STORE);

	}

	if(success == OK){

		block_size = atoi(block_sz);
		updated_local_file = fopen(file_name,"rb");
		compare_local_file(&listener, updated_local_file, checksums_received, block_size, BYTES_SIZE);
		fclose(updated_local_file);

	}

	end_connection(&listener);
	end_connection(&aceptor);
	
	free(checksums_received);

	return SYSTEM_EXIT;
}

int main(int argc, char *argv[]){

	//TODO: Validar que no hay errores de parametros!!

	if ( *(argv[POS_MODE]) == 'c' ){

		trigger_client_mode(argv);

	}else{

		trigger_server_mode(argv);

	}
}

