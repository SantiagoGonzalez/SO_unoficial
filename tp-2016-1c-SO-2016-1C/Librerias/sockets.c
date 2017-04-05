/*
 * sockets.c
 *
 *  Created on: 11/5/2016
 *      Author: utnso
 */
#include "sockets.h"

int crearServer(struct sockaddr_in* direccionServidor, int puerto) {

	direccionServidor->sin_family = AF_INET;
	direccionServidor->sin_addr.s_addr = INADDR_ANY;
	direccionServidor->sin_port = htons(puerto);

	int servidor = socket(AF_INET, SOCK_STREAM, 0);

	int activado = 1;
	setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

	return servidor;
}

void crearCliente(struct sockaddr_in* direccionServidor, int puerto, char* ip) {
	direccionServidor->sin_family = AF_INET;
	direccionServidor->sin_addr.s_addr = inet_addr(ip);
	direccionServidor->sin_port = htons(puerto);
}

int recibirCliente(int servidor) {
	struct sockaddr_in direccionCliente;
	unsigned int tamanioDireccion = sizeof(struct sockaddr_in);

	int cliente = accept(servidor, (void*) &direccionCliente,
			&tamanioDireccion);

	printf("Recibí una conexión del %d bytes \n", cliente);
	return cliente;
}

