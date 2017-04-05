#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <commons/config.h>
#include <sockets.h>
#include <estructuras.h>
#include <pthread.h>

// GLOBALES //
FILE* progAnsisop;
void* buffer;
int mandoPrograma;
int server;
// FIN GLOBALES //

void enviarAnsisop(int server, char* argumento){
	progAnsisop=fopen(argumento,"rb");
	if (progAnsisop == NULL){
		printf("No se pudo abrir el archivo\n");
		abort();
	}

	fseek(progAnsisop,0,SEEK_END);
	uint32_t sizeAnsisop=(ftell(progAnsisop)-115);
	fseek(progAnsisop,115,SEEK_SET);
	void*bufferArchivo=malloc(sizeAnsisop);
	fread(bufferArchivo,sizeAnsisop,1,progAnsisop);
	buffer=malloc(sizeAnsisop+(sizeof(uint32_t)*3));
	printf("%s\n",(char*)bufferArchivo);
	uint32_t idconsola= ID_CONSOLA;
	uint32_t orden = INICIALIZAR_ANSISOP;
	memcpy(buffer,&idconsola,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t),&orden,sizeof(uint32_t));
	memcpy(buffer+(sizeof(uint32_t)*2),&sizeAnsisop,sizeof(uint32_t));
	memcpy(buffer+(sizeof(uint32_t)*3),bufferArchivo,sizeAnsisop);
	send(server,buffer,((sizeof(uint32_t)*3)+sizeAnsisop),0);
	free(bufferArchivo);
}

void handshake(int socket) {
	uint32_t idCliente=ID_CONSOLA;
	send(socket,&idCliente,sizeof(uint32_t),0);
}

void mensajeConsola(){
	while(mandoPrograma==0);
		while(1){
			printf("Si desea finalizar el programa ingrese 1: \n");
			int finalizar_Programa;
			uint32_t id=ID_CONSOLA;
			uint32_t fin;
			scanf("%d",&finalizar_Programa);
			if(finalizar_Programa==1){
			void* bufferFin=malloc(sizeof(uint32_t)*3);
			fin=FINALIZAR_PROGRAMA_UMC;
			memcpy(buffer,&id,sizeof(uint32_t));
			memcpy(buffer+sizeof(uint32_t),&fin,sizeof(uint32_t));
			memcpy(buffer+(sizeof(uint32_t)*2),&id,sizeof(uint32_t));
			send(server,bufferFin,(sizeof(uint32_t)*3),0);
			free(buffer);
			free(bufferFin);
			fclose(progAnsisop);
			}
			else{
				printf("Numero no válido\n");
			}
		}
}

int main(int argc,char* argv[]) {
//Verifico cantidad de argumentos ingresados------------------------------------------------------------------------------------------------
	if(argc!=3){
		printf("Cantidad Invalida de argumentos\n");
	return 1;
	}
	mandoPrograma=0;
//Creo hilo de manejo de mensajes por Consola
	pthread_t	consola;
	pthread_attr_t atributo;
	pthread_attr_init(&atributo);
	pthread_attr_setdetachstate(&atributo,PTHREAD_CREATE_DETACHED);
	pthread_create(&consola,&atributo,(void*)mensajeConsola,NULL);
//Conexion con el Nucleo-------------------------------------------------------------------------------------------------------------------
	t_config* cfgCONSOLA= config_create(argv[1]);
	struct sockaddr_in direccionServidor;
	server = socket(AF_INET, SOCK_STREAM, 0);
	crearCliente(&direccionServidor,config_get_int_value(cfgCONSOLA,"PUERTO_NUCLEO"),config_get_string_value(cfgCONSOLA,"IP_NUCLEO"));
	if (connect(server, (void*) &direccionServidor, sizeof(direccionServidor))
			!= 0) {
		perror("No se pudo conectar al nucleo");
		return 1;
	}

//Verifico conexion con el Nucleo-----------------------------------------------------------------------------------------------------------
	handshake(server);

//Abro, copio y envio el codigo del Ansisop al Nucleo---------------------------------------------------------------------------------------
	enviarAnsisop(server,argv[2]);
	mandoPrograma=1;
	while(1){
		uint32_t orden;
		recv(server,&orden,sizeof(uint32_t),0);
		if((orden==IMPRIMIR)||(orden==IMPRIMIR_TEXTO)){
//Imprimo por pantalla lo que el Nucleo envia-----------------------------------------------------------------------------------------------
			uint32_t tamanioARecibir=0;
			recv(server,&tamanioARecibir,sizeof(int),0);
			buffer=calloc(1,tamanioARecibir);
			recv(server,buffer,tamanioARecibir,0);
			char*mostrar=malloc(tamanioARecibir);
			memcpy(mostrar,buffer,tamanioARecibir);
			printf("%s\n",mostrar);
		}
//Finalizar programa------------------------------------------------------------------------------------------------------------------------
		else{
			printf("El programa ha finalizado\n");
			free(buffer);
			fclose(progAnsisop);
			abort();
			}
		}

//Libero espacio de memoria utilizado-------------------------------------------------------------------------------------------------------
	free(buffer);
	fclose(progAnsisop);

	return 0;
}
