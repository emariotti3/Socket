#ifndef _BUFFER_D_H
#define _BUFFER_D_H

typedef struct buffer_d buffer_d_t;

struct buffer_d{
	size_t size;
	size_t saved;
	char *data;
};

//Initializes dynamic buffer structure.
//Receives initial buffer size. Returns pointer
//to initialized dynamic size buffer.
buffer_d_t* buffer_init(size_t size);

//Initializes dynamic buffer structure.
//Receives initial buffer size. Returns pointer
//to initialized dynamic size buffer.
//Returns true if resize was successful and
//false if it was unsuccessful.
bool buffer_resize(buffer_d_t *buffer);

//Receives pointer to dynamic buffer, a position and a char pointer.
//Stores the element fount at pos to address pointed by char *value.
bool buffer_get(buffer_d_t *buffer, size_t pos, char *value);

//Receives pointer to dynamic buffer, a position and a char.
//Stores the element stored in value address pointed at buffer[pos].
//Returns true if element was successfully stored.
bool buffer_save(buffer_d_t *buffer, char values[], size_t size);

//Receives pointer to dynamic buffer and 
//returns its current size. 
size_t buffer_get_size(buffer_d_t *buffer);

size_t buffer_get_saved(buffer_d_t *buffer);

//Receives pointer to dynamic buffer
//and destroys it.
void buffer_destroy(buffer_d_t *buffer);

#endif
