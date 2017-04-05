#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <commons/config.h>
#include <sockets.h>
#include <estructuras.h>
#include <errno.h>
#include <fcntl.h>
#include <commons/log.h>
#include <sys/mman.h>
#include <commons/bitarray.h>



// ESTRUCTURAS //---------------------------------------------------------

typedef struct{
	int PID;
	int estado;
}BitMap;

// GLOBALES //--------------------------------------------------------------------

FILE*archivoSwap;
t_log* logger;
char* nombre;
int tamPag;
int cantPag;
useconds_t retardoCompactacion;
BitMap bitMap[];
int exito=EXITO_SWAP;
int error=ERROR_SWAP;


// FUNCIONES //-------------------------------------------------------------------

int espacioDisponible(int paginas){
	int i=0, j=0;
	while(i<cantPag&&j!=paginas){
		if (bitMap[i].estado==0) {
			j++;
		}
		i++;
	}
	if(j==paginas)return 1;
	return 0;
}

int contiguo(int paginas){
	int i=0, j=0, fin;
	while((i<cantPag)&&(j!=paginas)){
		if((bitMap[i].estado==0)&&(bitMap[i+1].estado==0)){
			j++;
			fin=i;
		}
		else{
		j=0;
		}
		i++;
	}
	if(j==paginas)return (fin-paginas)+1;
	return -1;
}

void asignacion(int PID,int paginas, int inicio){
	int i;
	for (i = inicio; i < inicio+paginas; ++i) {
		bitMap[i].estado=1;
		bitMap[i].PID=PID;
	}

}

void compactar(){
	int i,j;
	BitMap aux;
	void* archivoAux=calloc(1,tamPag);
	void* archivoAux2=calloc(1,tamPag);
	for (j = 0;  j < cantPag; ++ j) {
		for (i = 0; i < cantPag-1; ++i) {
			if(bitMap[i].estado==0&&bitMap[i+1].estado==1){
				aux=bitMap[i];
				fseek(archivoSwap,i*tamPag,SEEK_SET);
				fread(archivoAux,tamPag,1,archivoSwap);
				bitMap[i]=bitMap[i+1];
				fseek(archivoSwap,(i+1)*tamPag,SEEK_SET);
				fread(archivoAux2,tamPag,1,archivoSwap);
				fseek(archivoSwap,i*tamPag,SEEK_SET);
				fwrite(archivoAux2,tamPag,1,archivoSwap);
				bitMap[i+1]=aux;
				fseek(archivoSwap,(i+1)*tamPag,SEEK_SET);
				fwrite(archivoAux,tamPag,1,archivoSwap);
			}
		}
	}
	free(archivoAux);
	free(archivoAux2);
}

void leer(OrdenProceso* reciboUMC,int cliente){
	int i=0;
	while((bitMap[i].PID!=reciboUMC->PID)&&(i<tamPag)){
		i++;
	}
	i=(i+reciboUMC->nroPagina);
	fseek(archivoSwap,i*tamPag,SEEK_SET);
	fread(reciboUMC->contenido,tamPag,1,archivoSwap);
	send(cliente,(void*)reciboUMC->contenido,strlen(reciboUMC->contenido),0);
}

void escribir(OrdenProceso* reciboUMC,int cliente){
	int i=0;
	while((bitMap[i].PID!=reciboUMC->PID)&&(i<cantPag)){
		i++;
	}
	if(bitMap[i].PID==reciboUMC->PID){
		i=(i+reciboUMC->nroPagina);
		fseek(archivoSwap,i*tamPag,SEEK_SET);
		fwrite(reciboUMC->contenido,strlen(reciboUMC->contenido),1,archivoSwap);
	}
}

void liberarMemoria(int PID,int cliente){
	int i=0;
	while(i<cantPag&&bitMap[i].PID!=PID){
		i++;
	}
	while(i<cantPag&&bitMap[i].PID==PID){
			bitMap[i].estado=0;
			bitMap[i].PID=-1;
			fseek(archivoSwap,i*tamPag,SEEK_SET);
			void* archivoAux=calloc(1,tamPag);
			fwrite(archivoAux,tamPag,1,archivoSwap);
			free(archivoAux);
			i++;
	}
	send(cliente,&exito,sizeof(int),0);
}

void realizarPedidoUMC(OrdenProceso* reciboUMC, int cliente){
	int inicio;
	switch(reciboUMC->orden){
		case(INICIAR):
				if(espacioDisponible(reciboUMC->cantPaginas)){
					if((inicio=contiguo(reciboUMC->cantPaginas))!=-1){
						asignacion(reciboUMC->PID,reciboUMC->cantPaginas,inicio);
						send(cliente,&exito,sizeof(int),0);
					}else{
						printf("Compactando\n");
						log_info(logger,"Compactando\n");
						compactar();
						usleep(retardoCompactacion*100);
						log_info(logger,"Compactacion finalizada\n");
						printf("Compactacion finalizada\n");
						inicio=contiguo(reciboUMC->cantPaginas);
						asignacion(reciboUMC->PID,reciboUMC->cantPaginas,inicio);
						send(cliente,&exito,sizeof(int),0);
					}
				}
				else{
					log_info(logger,"No hay espacio disponible\n");
					printf("No hay espacio disponible\n");
					send(cliente,&error,sizeof(int),0);
				}
			break;
	case(LEER):
			leer(reciboUMC,cliente);
			break;
	case(ESCRIBIR):
			escribir(reciboUMC,cliente);
			break;
	case(FINALIZAR):
			liberarMemoria(reciboUMC->PID,cliente);
			break;
	}
}

void copiarCONTENIDO(OrdenProceso* reciboUMC, void* aux){
	memcpy((reciboUMC->contenido),aux+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t),tamPag);
	log_info(logger,"Contenido: %s\n",(reciboUMC->contenido));
	printf("Contenido: %s\n",(reciboUMC->contenido));
}

void copiarNroPAGINA(OrdenProceso* reciboUMC, void* aux){
	memcpy(&(reciboUMC->nroPagina),aux+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t),sizeof(uint32_t));
	log_info(logger,"Numero de pagina: %d\n",reciboUMC->nroPagina);
	printf("Numero de pagina: %d\n",reciboUMC->nroPagina);
}

void copiarCantPaginas(OrdenProceso* reciboUMC, void* aux){
	memcpy(&(reciboUMC->cantPaginas),aux+sizeof(uint32_t)+sizeof(uint32_t),sizeof(uint32_t));
	log_info(logger,"Cantidad de paginas: %d\n",reciboUMC->cantPaginas);
	printf("Cantidad de paginas: %d\n",reciboUMC->cantPaginas);
}

void copiarORDEN(OrdenProceso* reciboUMC, void* aux){
	memcpy(&(reciboUMC->orden),aux+sizeof(uint32_t),sizeof(uint32_t));
	log_info(logger,"Orden: %d\n",reciboUMC->orden);
	printf("Orden: %d\n",reciboUMC->orden);
}

void copiarPID(OrdenProceso* reciboUMC, void* aux){
	memcpy(&(reciboUMC->PID),aux,sizeof(uint32_t));
	log_info(logger,"PID: %d\n",reciboUMC->PID);
	printf("PID: %d\n",reciboUMC->PID);
}

void copiar(OrdenProceso* reciboUMC, void* aux){
	copiarPID(reciboUMC, aux);
	copiarORDEN(reciboUMC, aux);
	copiarCantPaginas(reciboUMC, aux);
	copiarNroPAGINA(reciboUMC, aux);
	copiarCONTENIDO(reciboUMC, aux);
}

void handShake_SWAP(uint32_t idCliente,int cliente) {
	if (idCliente==ID_UMC) {
		printf("Recibi a la UMC\n");
	}
	else{
		printf("No es lo que esperaba\n");
		close(cliente);
	}
}

void handshake(int cliente,uint32_t idReceptor) {
	uint32_t idCliente;
	recv(cliente, &idCliente, sizeof(uint32_t), 0);
	if (idReceptor==ID_SWAP) {
		handShake_SWAP(idCliente,cliente);
	}
}

void crearYAbrirArchivoSwap(t_config* cfgSWAP){

	nombre=config_get_string_value(cfgSWAP,"NOMBRE_SWAP");
	cantPag=config_get_int_value(cfgSWAP,"CANTIDAD_PAGINAS");
	tamPag=config_get_int_value(cfgSWAP,"TAMANIO_PAGINAS");

	int tamArch=cantPag*tamPag;
	char tamanioArch[10];
	char comando[100]= "dd if=/dev/zero of=./";

	strcat(comando,nombre);
	strcat(comando," bs=");
	sprintf(tamanioArch,"%d",tamArch);
	strcat(comando,tamanioArch);
	strcat(comando," count=1");

	if ( system(comando) !=0){
		log_info(logger,"No se creo el archivo\n");
		printf("No se creo el archivo\n");
		abort();
	}
	log_info(logger,"Particion creada satisfactoriamente\n");
	printf("Particion creada satisfactoriamente\n");

	archivoSwap=fopen("./swap.data","w+");

	if (archivoSwap == NULL){
		log_info(logger,"No se pudo abrir el archivo\n");
		printf("No se pudo abrir el archivo\n");
		abort();
	}
	else{
		char a[]="\0";
		fwrite(a,1,sizeof(a),archivoSwap);
		log_info(logger,"Particion inicializada satisfactoriamente\n");
		printf("Particion inicializada satisfactoriamente\n");
	}
}
void inicializarBitMap(){
	int i;
	for (i = 0; i < cantPag; ++i) {
		bitMap[i].estado=0;
		bitMap[i].PID=-1;
	}
}


int main(int argc,char* argv[]) {
	if(argc!=2){
		printf("Cantidad Invalida de argumentos\n");
	return 1;
	}
	t_config* cfgSWAP= config_create(argv[1]);
	nombre=malloc(20);
	retardoCompactacion=config_get_int_value(cfgSWAP,"RETARDO_COMPACTACION");

	bitMap[cantPag];

	inicializarBitMap();

//Creo archivo Log------------------------------------------------------------------------------------------------
	logger=log_create("logSwap","SWAP",1,1);
// Creo y abro Archivo Swap para memoria virtual------------------------------------------------------------------
	crearYAbrirArchivoSwap(cfgSWAP);

// Crear Servidor------------------------------------------------------------------------------------------------
	struct sockaddr_in direccionServidor;
	int servidor= crearServer(&direccionServidor, config_get_int_value(cfgSWAP,"PUERTO_SWAP"));

	if (bind(servidor, (void*) &direccionServidor, sizeof(direccionServidor)) != 0) {
		perror("Fallo el bind");
		//return 1;
	}

	log_info(logger,"Estoy escuchando, soy SWAP\n");
	printf("Estoy escuchando, soy SWAP\n");

// Espero conexion del UMC----------------------------------------------------------------------------------------
	listen(servidor, SOMAXCONN);

// Recivo conexion------------------------------------------------------------------------------------------------
	char* buffer = malloc(50);

	int cliente= recibirCliente(servidor);

// Verifico que la conexion sea de la UMC-------------------------------------------------------------------------
	handshake(cliente,ID_SWAP);

 // Recibo orden de UMC y la proceso--------------------------------------------------------------------
		int i=1;
		uint32_t tamanioARecibir=0;
		OrdenProceso* reciboUMC=malloc(sizeof(OrdenProceso));
		reciboUMC->contenido=calloc(1,tamPag);
		void*aux;

	while(i){
		recv(cliente,&tamanioARecibir,sizeof(uint32_t),0);

		aux=malloc(tamanioARecibir);
		i= recv(cliente,aux,tamanioARecibir,0);

		if(i>0){
			copiar(reciboUMC,aux);

			realizarPedidoUMC(reciboUMC,cliente); // Inicializar/Finalizar/Lectura/Escritura

			free(aux);
		}
		else{
			log_info(logger,"Hubo un problema con la UMC\n");
			printf("Hubo un problema con la UMC\n");
			close(cliente);
			return 0;
		}
	}

	free(buffer);
	fclose(archivoSwap);
	log_destroy(logger);
	return 0;
}
