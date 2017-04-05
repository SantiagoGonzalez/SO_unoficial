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
#include "variablesGlobalesUMC.h"
#include "funciones.h"

/*void *get_in_addr(struct sockaddr *sa) {
 if (sa->sa_family == AF_INET) {
 return &(((struct sockaddr_in*) sa)->sin_addr);
 }

 return &(((struct sockaddr_in6*) sa)->sin6_addr);
 }*/

void handShake_UMC(uint8_t cliente, int* n) {
	switch ((int) cliente) {
	case ID_NUCLEO:
		*n = 2;
		break;
	case ID_CPU:
		*n = 1;
		break;
	default:
		printf("No es lo que esperaba\n");
		*n = 0;
		break;
	}
}

int handShake(int socket, uint32_t idReceptor) {
	uint32_t idCliente;
	recv(socket, &idCliente, sizeof(uint32_t), 0);
	int n=0;
	switch ((int) idReceptor) {
	case ID_UMC:
		handShake_UMC(idCliente, &n);
		break;
	}
		return n;
}

/*void* escucharCPUs (void* arg){
 char* buffer=malloc(50);
 int socket= (int)arg;
 listen(socket,100);
 while(1){
 int cliente2 = recibirCliente(socket);
 printf("Recibí una conexión en %d!!\n", cliente2);
 int bytes=recv(cliente2,buffer,50,0);
 handShake(buffer[0],ID_UMC);
 }
 }*/

void operacionesCPU(void* socket) {
	int cliente = (int) socket;
	int bytes;
	uint32_t orden;
	uint32_t pid=0;
	uint32_t tamanioPagina=tamanioFrame;
	send(cliente,&tamanioPagina,sizeof(uint32_t),0);
	while(1){
	bytes=recv(cliente, &orden, sizeof(uint32_t), 0);
	if(bytes<=0){
		log_error(logger,"se cayo el CPU correspondiente al socket: %d",cliente);
		pthread_exit(NULL);
	}
	else{
	switch ((int)orden) {
	case SOLICITAR_BYTES_UMC: {
		uint32_t numpag;
		uint32_t offset;
		uint32_t tamanio;
		recv(cliente,&numpag,sizeof(sizeof(uint32_t)),0);
		recv(cliente,&offset,sizeof(sizeof(uint32_t)),0);
		recv(cliente,&tamanio,sizeof(sizeof(uint32_t)),0);
		solicitarBytes((int)numpag,(int)offset,(int)tamanio,pid,cliente);
		break;
	}
	case ALMACENAR_BYTES_UMC: {
		uint32_t numpag;
		uint32_t offset;
		uint32_t tamanio;
		uint32_t valorReal;
		recv(cliente,&numpag,sizeof(sizeof(uint32_t)),0);
		recv(cliente,&offset,sizeof(sizeof(uint32_t)),0);
		recv(cliente,&tamanio,sizeof(sizeof(uint32_t)),0);
		recv(cliente,&valorReal,sizeof(sizeof(uint32_t)),0);
		almacenarBytesPagina((int)numpag,(int)offset,(int)tamanio,(void*)valorReal,pid,cliente);
		break;
	}
	case CAMBIO_DE_PROCESO:{
		uint32_t pidNuevo;
		recv(cliente,&pidNuevo,sizeof(uint32_t),0);
		cambioProceso(pidNuevo,&pid);
	}
	}
	}
	}
}
void operacionesNUCLEO(){
	uint32_t numeroOp;
	int bytes;
	uint32_t tamanioPagina=tamanioFrame;
	//pthread_mutex_lock(&mutexNucleo);
	send(socketNucleo,&tamanioPagina,sizeof(uint32_t),0);
	//pthread_mutex_unlock(&mutexNucleo);
	while(1){
		//pthread_mutex_lock(&mutexNucleo);
	//printf("%d",socketNucleo);
	bytes=recv(socketNucleo,&numeroOp,sizeof(uint32_t),0);
	//pthread_mutex_unlock(&mutexNucleo);
	if(bytes<=0){
		log_info(logger,"Se desconecto el nucleo, cerrando UMC");
		abort();
	}
	else{
	switch(numeroOp){
	case INICIALIZAR_PROGRAMA_UMC:{
		uint32_t pid;
		uint32_t cantPag;
		uint32_t tamanioCodigo;
		recv(socketNucleo,&pid,sizeof(uint32_t),0);
		recv(socketNucleo,&cantPag,sizeof(uint32_t),0);
		recv(socketNucleo,&tamanioCodigo,sizeof(uint32_t),0);
		void*buffer=malloc(tamanioCodigo);
		recv(socketNucleo,buffer,tamanioCodigo,0);
		log_info(logger,"Nucleo solicita Inicializacion del programa con pid: %d",pid);
		inicializarPrograma(pid,(int)cantPag,buffer);
		break;
	}
	case FINALIZAR_PROGRAMA_UMC:{
			int pid;
			recv(socketNucleo,&pid,sizeof(int),0);
			log_info(logger,"Nucleo solicita Finalizacion del programa con pid: %d",pid);
			finalizarPrograma((uint32_t)pid);
			break;
		}
	}
	}
	}
}

void listenerConsola() {
	char *mensaje = malloc(30);
	//size_t bytes = 30;
	while (1) {
		//getline(&mensaje, &bytes, stdin);
		printf("\n");
		scanf("%s",mensaje);
		if (!strcmp(mensaje, "flush")) {
			printf("Sobre que quiere hacer flush?\n-tlb\n-memory");
			scanf("%s",mensaje);
			if(!strcmp(mensaje,"tlb")){
				printf("Flush TLB\n");
				flushTLB();
			}
			else if(!strcmp(mensaje,"memory")){
				printf("Flush Memoria\n");
				flushMemoria();
			}
		}
		else if(!strcmp(mensaje,"dump")){
			printf("Dump\n");
			printf("Como quiere hacer dump?\n-total\n-pid");
						scanf("%s",mensaje);
						if(!strcmp(mensaje,"total")){
							printf("Dump Total\n");
							dumpTotal();
						}
						else if(!strcmp(mensaje,"pid")){
							printf("ingrese PID\n");
							char*mensaje2=malloc(sizeof(uint32_t));
							scanf("%s",mensaje2);

							dumpPID((uint32_t)mensaje2);
						}
		}
		else if(!strcmp(mensaje,"retardo")){
			printf("Retardo\n");
			//RetardoUMC
		}
		else{
			printf("Comando no valido\n");
		}
	}

}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("Cantidad Invalida de argumentos\n");
		return 1;
	}
	uint32_t id=ID_UMC;
	pthread_mutex_init(&mutexMarcos, NULL);
	pthread_mutex_init(&mutexAccesoMem, NULL);
	pthread_mutex_init(&mutexProcs, NULL);
	pthread_mutex_init(&mutexRetardo,NULL);
	pthread_mutex_init(&mutexMP,NULL);
	pthread_mutex_init(&mutexSwap, NULL);
	pthread_mutex_init(&mutexTLB, NULL);
	pthread_mutex_init(&mutexNucleo, NULL);
	tlb = list_create();
	t_config* cfgUMC = config_create(argv[1]);
	struct sockaddr_in direccionSwap;
	crearCliente(&direccionSwap,config_get_int_value(cfgUMC, "PUERTO_SWAP"),config_get_string_value(cfgUMC,"IP_SWAP"));
	socketSwap=socket(AF_INET, SOCK_STREAM, 0);
	logger=log_create("logsUMC","UMC",1,1);
	if(connect(socketSwap,(void*)&direccionSwap,sizeof(direccionSwap))!=0){
		log_error(logger,"No se pudo conectar al SWAP");
		abort();
	}
	else{
	send(socketSwap,&id,sizeof(uint32_t),0);
	log_info(logger,"Se conecto correctamente al SWAP");
	inicializarListasYVariables(cfgUMC);
	struct sockaddr_in direccionServidor;
	int socketServ = crearServer(&direccionServidor,
			config_get_int_value(cfgUMC, "PUERTO_UMC")); //Creo un SV y devuelvo el socket
	if (bind(socketServ, (void*) &direccionServidor, sizeof(direccionServidor))
			!= 0) {
		perror("Falló el bind");
		return 1;
	}
	log_info(logger,"UMC Escuchando");
	int thread_consola;
	pthread_t hiloConsola;
	pthread_attr_t atributo;
	pthread_attr_init(&atributo);
	pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);
	thread_consola = pthread_create(&hiloConsola, &atributo,
			(void*) listenerConsola, NULL);
	log_info(logger,"Espero al Nucleo");
	listen(socketServ, 100);

	int validarNucleo = 0;
	while (validarNucleo == 0) {
		socketNucleo = recibirCliente(socketServ);
		int validacion = handShake(socketNucleo, ID_UMC);
		if (validacion == 2) {
			validarNucleo = 1;
			log_info(logger,"Se conecto el nucleo");
			int thread_escuchaNUCLEO;
			pthread_t hiloEscuchaNUCLEO;
						pthread_attr_t atributo;
						pthread_attr_init(&atributo);
						pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);
						thread_escuchaNUCLEO = pthread_create(&hiloEscuchaNUCLEO, &atributo,
								(void*) operacionesNUCLEO, NULL);
						log_info(logger,"Se creo el hilo %d para el nucleo",thread_escuchaNUCLEO);
						pthread_attr_destroy(&atributo);
		} else {
			close(socketNucleo);
		}
	}
	log_info(logger,"Espero a las CPUs");
	while (1) {
		int cliente = recibirCliente(socketServ);
		int validar = handShake(cliente, ID_UMC);
		if (validar == 1) {
			log_info(logger,"Llego una CPU en el socket %d", cliente);
			int thread_escuchaCPU;
			pthread_t hiloEscuchaCPUs;
			pthread_attr_t atributo;
			pthread_attr_init(&atributo);
			pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);
			thread_escuchaCPU = pthread_create(&hiloEscuchaCPUs, &atributo,
					(void*) operacionesCPU, (void*) cliente);
			log_info(logger,"Se creo el hilo %d para un CPU",thread_escuchaCPU);
			pthread_attr_destroy(&atributo);
		} else if (validar == 0) {
			close(cliente);
		}
	}
	/* int cliente = recibirCliente(servidor);
	 int bytesRecibidos = recv(cliente, buffer,
	 50, 0);

	 int id=(int)buffer[0];

	 handShake(id,ID_UMC);
	 if (bytesRecibidos <= 0) {
	 perror("Se ha perdido contacto ");
	 }*/

	free(memoriaprincipal);
	log_destroy(logger);
	return 0;
	}
}
