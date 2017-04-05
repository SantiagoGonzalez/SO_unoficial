/*
 * sockets.h
 *
 *  Created on: 11/5/2016
 *      Author: utnso
 */

#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>


int crearServer(struct sockaddr_in* direccionServidor, int puerto);

void crearCliente(struct sockaddr_in* direccionServidor, int puerto, char* ip);

int recibirCliente(int servidor);

#endif /* SOCKETS_H_ */
