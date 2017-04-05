#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void crearServer(struct sockaddr_in* direccionServidor, int puerto) {
	direccionServidor->sin_family = AF_INET;
	direccionServidor->sin_addr.s_addr = INADDR_ANY;
	direccionServidor->sin_port = htons(puerto);
}

int main(void) {
	struct sockaddr_in direccionServidor;
	crearServer(&direccionServidor,8080);

	int servidor = socket(AF_INET, SOCK_STREAM, 0);

	int activado = 1;

	setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

	if (bind(servidor, (void*) &direccionServidor, sizeof(direccionServidor)) != 0) {
		perror("Falló el bind");
		return 1;
	}

	printf("Estoy escuchando\n");
	listen(servidor, 100);

	//------------------------------

	struct sockaddr_in direccionCliente;
	unsigned int tamanioDireccion;
	int cliente = accept(servidor, (void*) &direccionCliente, &tamanioDireccion);

	printf("Recibí una conexión en %d!!\n", cliente);
	send(cliente, "Hola NetCat!", 13, 0);
	send(cliente,"aaaa",5,0);

	//------------------------------

	char* buffer = malloc(1000);

	while (1) {
//		envio
		char* mensaje[1000];


//-------------------------llegada------------------------------
		int bytesRecibidos = recv(cliente, buffer, 1000, 0);
		if (bytesRecibidos <= 0) {
			perror("El chabón se desconectó o bla.");
			return 1;
		}

		buffer[bytesRecibidos] = '\0';
		printf("Me llegaron %d bytes con %s\n", bytesRecibidos, buffer);
//---------------------------------------------------------------


//--------------------envio-------------

		scanf("%s", mensaje);
		send(cliente, mensaje, strlen(mensaje), 0);

	}

	free(buffer);

	return 0;
}
