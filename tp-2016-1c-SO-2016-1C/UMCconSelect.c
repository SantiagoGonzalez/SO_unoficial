#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <commons/config.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sockets.h>
#include <estructuras.h>
#include <pthread.h>
#define PORT "8100"

/*void *get_in_addr(struct sockaddr *sa) {
 if (sa->sa_family == AF_INET) {
 return &(((struct sockaddr_in*) sa)->sin_addr);
 }

 return &(((struct sockaddr_in6*) sa)->sin6_addr);
 }*/

void operacionesCPU(void* num_op){

	switch((int)num_op){
	case 1:
		deserializar_PCB();
		break;
	default:
		printf("Que e eso?");
	}
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("Cantidad Invalida de argumentos\n");
		return 1;
	}
	t_config* cfgUMC = config_create(argv[1]);
	int tamanio_mp = (config_get_int_value(cfgUMC, "TAMAÑO_FRAME"))
			* (config_get_int_value(cfgUMC, "CANT_FRAMES"));
	void* mp = malloc(tamanio_mp);
	pthread_attr_t atributo;
	pthread_attr_init(&atributo);
	pthread_attr_setdetachstate(&atributo,PTHREAD_CREATE_DETACHED);

	struct sockaddr_in direccionServidor;

	fd_set master;   // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	//struct sockaddr_in myaddr;     // dirección del servidor
	struct sockaddr_in remoteaddr; // dirección del cliente
	int fdmax;        // número máximo de descriptores de fichero
	int listener;     // descriptor de socket a la escucha
	int newfd;        // descriptor de socket de nueva conexión aceptada
	char *buf = malloc(50);    // buffer para datos del cliente
	int nbytes;
	int yes = 1;        // para setsockopt() SO_REUSEADDR, más abajo
	int addrlen;
	int i, j;
	FD_ZERO(&master);    // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);
	// obtener socket a la escucha
	if ((listener = crearServer(&direccionServidor,
			config_get_int_value(cfgUMC, "PUERTO_UMC"))) == -1) {
		perror("socket");
		exit(1);
	}
	// obviar el mensaje "address already in use" (la dirección ya se está usando)
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) {
		perror("setsockopt");
		exit(1);
	}
	// enlazar
	memset(&(direccionServidor.sin_zero), '\0', 8);
	if (bind(listener, (void*) &direccionServidor, sizeof(direccionServidor))
			== -1) {
		perror("bind");
		exit(1);
	}
	// escuchar
	if (listen(listener, 10) == -1) {
		perror("listen");
		exit(1);
	}
	// añadir listener al conjunto maestro
	FD_SET(listener, &master);
	// seguir la pista del descriptor de fichero mayor
	fdmax = listener; // por ahora es éste
	// bucle principal
	for (;;) {
		read_fds = master; // cópialo
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}
		// explorar conexiones existentes en busca de datos que leer
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
				if (i == listener) {
					// gestionar nuevas conexiones
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(listener,
							(struct sockaddr *) &remoteaddr, &addrlen)) == -1) {
						perror("accept");
					} else {
						FD_SET(newfd, &master); // añadir al conjunto maestro
						if (newfd > fdmax) {    // actualizar el máximo
							fdmax = newfd;
						}

						printf("selectserver: new connection from %s on "
								"socket %d\n", inet_ntoa(remoteaddr.sin_addr),
								newfd);
					}
				} else {
					// gestionar datos de un cliente
					if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
						printf("%d",nbytes);
						if (nbytes == 0) {
							// conexión cerrada
							printf("selectserver: socket %d hung up\n", i);
						} else {
							perror("recv");
						}
						close(i); // bye!
						FD_CLR(i, &master); // eliminar del conjunto maestro
					} else {
						// tenemos datos de algún cliente y aca va el tp
						int id = (int) buf[0];

						switch (id) {
						case ID_CPU: {
							//Crear nuevo hilo

							pthread_t hiloNuevo;
							printf("Recibi un CPU\n");
							int t;
							t=pthread_create(&hiloNuevo,&atributo,(void*)*operacionesCPU,(void*)&buf[1]);
							while(1);
							printf("Recibi un CPU\n");
							printf("Mando algo al CPU\n");
							send(i, "Bienvenido a la UMC", 21, 0);
							break;
						}
						case ID_NUCLEO: {
							//Crear hilo nucleo
							printf("Recibi un nucleo(?\n");
							send(i, "Bienvenido a la UMC", 20, 0);
							break;
						}
						default: {
							printf("No es algo que espero\n");
							send(i, "Pica de aca ameo", 25, 0);
							close(i);
							FD_CLR(i, &master);
							printf("Y volo volo\n");
						}
						}

					}
				}
			}
		}
	}
	pthread_attr_destroy(&atributo);
	free(buf);
	free(mp);
	return 0;
}

