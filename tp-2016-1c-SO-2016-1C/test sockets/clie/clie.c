#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void crearCliente(struct sockaddr_in* direccionServidor, int puerto, char* ip) {

	direccionServidor->sin_family = AF_INET;
	direccionServidor->sin_addr.s_addr = inet_addr(ip);
	direccionServidor->sin_port = htons(puerto);
}

int main(void) {
	char* buffer = malloc(1000);
	struct sockaddr_in direccionServidor;

	crearCliente(&direccionServidor, 8080, "127.0.0.1");

	int server = socket(AF_INET, SOCK_STREAM, 0);

	if (connect(server, (void*) &direccionServidor, sizeof(direccionServidor))
			!= 0) {
		perror("No se pudo conectar");
		return 1;

	} else {
		int bytesRecibidos = recv(server, buffer, 13, 0);
		if (bytesRecibidos <= 0) {
			perror("El servidor se cayó.");
			return 1;
		}
		printf("%s", buffer);
	}

//	char* bufferLlegada = malloc(1000);

	while (1) {
//		envio
		char mensaje[1000];

//--------------llegada---------------------
//		int bytesRecibidos = recv(server, buffer, 1000, 0);
//		if(bytesRecibidos>0){
//
//		buffer[bytesRecibidos] = '\0';
//		printf("Me llegaron %d bytes con %s\n", bytesRecibidos, buffer);
//		}
//-------------------------------------------------

//		envio
		scanf("%s", mensaje);
		send(server, mensaje, strlen(mensaje), 0);

	}
	free(buffer);
	return 0;
}
