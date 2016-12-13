#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "dynamic_buffer.h"

buffer_d_t *buffer_init(size_t size){
	buffer_d_t *self = malloc(sizeof(buffer_d_t));

	if (self == NULL){
		return NULL;
	}
	self->data = malloc(size * sizeof(char));

	if (self->data == NULL){
	    free(self);
	    return NULL;
	}
	self->size = size;
	self->saved = 0;
	return self;
}

size_t buffer_get_size(buffer_d_t *self){ 
	return self->size;
}

size_t buffer_get_saved(buffer_d_t *self){ 
	return self->saved;
}

bool buffer_resize(buffer_d_t *self){
	size_t new_size = 2*(self->size);
	char *new_data = realloc(self->data, new_size*sizeof(char));

	if ((new_data == NULL) && (new_size > 0)){
	    return false;
	}
	self->data = new_data;
	self->size = new_size;

	return true;
}

bool buffer_get(buffer_d_t *self, size_t pos, char *value){       
	int buffer_size = self->size;
    int max_pos = buffer_size - 1;

    if ((pos <= max_pos) && (buffer_size > 0)){
        *value = (self->data)[pos];
        return true;
    } 
    return false;
}

bool buffer_save(buffer_d_t *self, char values[], size_t size_values){
    int buffer_size = self->size;
    int max_pos = self->saved + size_values -1;
    size_t pos = self->saved;
    int i = 0;
    if ((max_pos < buffer_size) && (buffer_size > 0)){
    	while(pos <= max_pos){
    		(self->data)[pos] = *(values + i);
    		pos++;	
    		i++;
    	}
        self->saved += size_values;
        return true;
    } 	
    return false;
}

void buffer_destroy(buffer_d_t *buffer){   
	free(buffer->data); 
	free(buffer);
}

