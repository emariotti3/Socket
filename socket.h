#ifndef SOCKET_H
#define SOCKET_H

typedef struct socket socket_t;

struct socket 
{
	int file_descriptor;
};

//Recibe un puntero a socket y un puntero a struct addrinfo
//inicializa el socket, devolviendo 0 si todo salió bien
//o !=0 si ocurrió un error.
int socket_init(socket_t *self, struct addrinfo *addr_info);


//Recibe un puntero a socket
//y libera los recursos asignados al mismo
//cerrando la conexión en forma permanente.
void socket_destroy(socket_t *self);


void socket_shutdown(socket_t *self);

//Recibe un socket s, un buffer de chars y su tamaño.
//Envía los chars contenidos en el buffer y su longitud 
//a través del socket s. Devuelve 0 si el envío se realizó correctamente
//o !=0 si ocurrió un error.
int socket_send(socket_t *self, char *buffer, unsigned int size);

//Recibe un socket s, un buffer de chars y su tamaño.
//Recibe chars a través del socket self y el tamaño que debe tener 
//el buffer para contenerlos. Devuelve 0 si todos los bytes
//se recibieron correctamente y !=0 si ocurrió un error.
int socket_receive(socket_t *self, char *buffer, size_t size);

//Acepta el pedido de conexión del cliente.
//Inicializa un nuevo socket_conexion que se
//encargará de manejar dicha conexión.
void socket_accept(socket_t *self, socket_t *connection_socket);

//Envía un pedido de conexión al socket 
//servidor.
int socket_connect(socket_t *server, struct addrinfo *addr_info);

//Recibe un puntero a socket_t y lo liga a un puerto
//y una dirección ip. También establece la cantidad
//máxima de conexiones que puede haber en espera para ser aceptadas.
int socket_bind_and_listen(socket_t *self, struct addrinfo *addr_info);

#endif