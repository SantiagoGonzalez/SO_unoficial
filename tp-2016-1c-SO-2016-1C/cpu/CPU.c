#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/socket.h>
#include <commons/config.h>
#include <commons/string.h>
#include <sockets.h>
#include <estructuras.h>
#include <parser/parser.h>
#include <parser/metadata_program.h>
#include "primitivas.h"
#include "globales.h"

AnSISOP_funciones functions= {
		.AnSISOP_definirVariable		= definirVariable,
		.AnSISOP_obtenerPosicionVariable = obtenerPosicionVariable,
		.AnSISOP_dereferenciar			= dereferenciar,
		.AnSISOP_asignar				= asignar,
		.AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
		.AnSISOP_asignarValorCompartida = asignarValorCompartida,
		.AnSISOP_irAlLabel				= irAlLabel,
		.AnSISOP_llamarConRetorno		= llamarConRetorno,
		.AnSISOP_retornar				= retornar,
		.AnSISOP_imprimir				= imprimir,
		.AnSISOP_imprimirTexto			= imprimirTexto,
		.AnSISOP_entradaSalida			= entradaSalida,
		.AnSISOP_finalizar				= finalizar,
};

AnSISOP_kernel kernel_functions = {
		.AnSISOP_wait					= wait,
		.AnSISOP_signal					= signal_primitiva,
};


void parsearLinea(char*linea){
	printf("Parseo la linea\n");
	analizadorLinea(linea,&functions,&kernel_functions);
	printf("Linea parseada\n");
}

void handshake(uint32_t socket){
	switch (socket) {
		case ID_UMC:
			send(umc,&id_cpu,sizeof(id_cpu),0);
			break;
		case ID_NUCLEO:
			send(nucleo,&id_cpu,sizeof(id_cpu),0);
			break;
	}

}

void obtenerTamanioPagina(int umc){
	recv(umc,&tamanioPagina,sizeof(uint32_t),0);
}

void conexionUMC(){
	struct sockaddr_in direccionServidorUMC;
		crearCliente(&direccionServidorUMC, config_get_int_value(cfgCPU,"PUERTO_UMC"), config_get_string_value(cfgCPU,"IP_UMC"));
		umc = socket(AF_INET, SOCK_STREAM, 0);
		if (connect(umc, (void*) &direccionServidorUMC, sizeof(direccionServidorUMC))!= 0) {
			perror("No se pudo conectar con UMC\n");
			abort();
		}else {
			printf("Me conecte a UMC\n");
			handshake((uint32_t)ID_UMC);
			obtenerTamanioPagina(umc);
			printf("Obtuve tamanio de Pagina de UMC: %d\n",tamanioPagina);
			}
}

void conexionNucleo(){
	struct sockaddr_in direccionServidorNucleo;
		crearCliente(&direccionServidorNucleo,config_get_int_value(cfgCPU,"PUERTO_NUCLEO"),config_get_string_value(cfgCPU,"IP_NUCLEO"));
		nucleo = socket(AF_INET, SOCK_STREAM, 0);
		if (connect(nucleo, (void*) &direccionServidorNucleo, sizeof(direccionServidorNucleo))!= 0) {
			perror("No se pudo conectar con núcleo\n");
			abort();
		}else {
			handshake((uint32_t) ID_NUCLEO);
			sigo_ejecutando=1;
			printf("Me conecte a Nucleo\n");
				}
}

int calcularNumPag(){
	return (pcb->indiceDeCodigo[pcb->PC].start) / (tamanioPagina);
}

int calcularOffset(){
	return (pcb->indiceDeCodigo[pcb->PC].start)%(tamanioPagina);
}

//ID_CPU,PASO_QUANTUM,un 0
void notificarNucleoQuantum(int nucleo){

	int tamAEnviar=sizeof(uint32_t)*3;
	uint32_t cero=0;
	void*buffer=malloc(tamAEnviar);
	memcpy(buffer,&id_cpu,sizeof(uint32_t));
	uint32_t orden= (uint32_t) PASO_QUANTUM;
	memcpy(buffer+sizeof(uint32_t),&orden,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t),&cero,sizeof(uint32_t));
	send(nucleo,buffer,tamAEnviar,0);
	printf("Notifique al nucleo que paso un quantum\n");
	free(buffer);
	uint32_t resultado;
	recv(nucleo,&resultado,sizeof(uint32_t),0);
	void*buffer2=malloc(60000);
	switch (resultado) {
		case DEVOLVER_PCB:
			printf("EL nucleo me pidio devolverle el PCB\n");
			enviarPCBparaNucleo(nucleo,(uint32_t) FIN_QUANTUM,buffer2);
			sigo_ejecutando=0;
			break;
		case SEGUIR_EJECUTANDO:
			printf("El nucleo me dijo que siga ejecutando\n");
			sigo_ejecutando=1;
			break;
	}

}

void atenderSenial(int senial) {
	if (!sigo_ejecutando) {
		printf("Me llego la señal SIGUR1\n");
		printf("EXIT\n");
		exit(1);
	}
	fin_cpu = 1;
}

void finalizarCPU(){
	printf("Finaliza CPU\n");
	uint32_t fincpu=(uint32_t) FIN_CPU;
	void*buffer=malloc(60000);
	tamanioHastaAhora=0;
	agregarPBCYEnviar(nucleo,buffer);
	void*buffer2=malloc(sizeof(uint32_t)*3);
	tamanioHastaAhora=0;
	memcpy(buffer2,&id_cpu,sizeof(uint32_t));
	tamanioHastaAhora+=sizeof(uint32_t);
	memcpy(buffer2+tamanioHastaAhora,&fincpu,sizeof(uint32_t));
	tamanioHastaAhora+=sizeof(uint32_t);
	uint32_t cero=0;
	memcpy(buffer2+tamanioHastaAhora,&cero,sizeof(uint32_t));
	tamanioHastaAhora+=sizeof(uint32_t);
	send(nucleo,buffer2,tamanioHastaAhora,0);
	printf("Envio al nucleo el PCB\n");
	free(buffer2);
}

//Orden CAMPIO PROC + PCB->PID
void enviarCambioProcesoUMC(){
	void*buffer=malloc(sizeof(uint32_t)*2);
	tamanioHastaAhora=0;
	uint32_t cambioproc=(uint32_t) CAMBIO_DE_PROCESO;
	memcpy(buffer,&cambioproc,sizeof(uint32_t));
	tamanioHastaAhora+=sizeof(uint32_t);
	memcpy(buffer+tamanioHastaAhora,&(pcb->PID),sizeof(uint32_t));
	tamanioHastaAhora+=sizeof(uint32_t);
	send(umc,buffer,tamanioHastaAhora,0);
	printf("Se envio cambio de Proceso a UMC\n");
	free(buffer);
}

int main(int argc,char* argv[]) {
	if(argc!=2){
		printf("Cantidad Invalida de argumentos\n");
	return 1;
	}

	cfgCPU=config_create(argv[1]);
	id_cpu=(uint32_t)ID_CPU;

//Conexion UMC-----------------recibo tamPag---------------------
	conexionUMC();

//Conexion Nucleo--------------recibo PCB-----------------------
	conexionNucleo();


	signal(SIGUSR1,atenderSenial);

	while(!fin_cpu){

		recibirPCBparaCPU(nucleo);
		printf("%d\n",pcb->PC);
		//pcb->PC++;
		printf("%d\n",pcb->PC);
		enviarCambioProcesoUMC();

		while (sigo_ejecutando){
			uint32_t numPag=(uint32_t) calcularNumPag();
			uint32_t offset=(uint32_t) calcularOffset();
			uint32_t tamanio=  (pcb->indiceDeCodigo[pcb->PC+1].offset);//Chequear el +1 negro que esta ahi
			void* linea;

			//Obtengo Linea a Parsear--------------------------------------
			printf("Solicito a la UMC la linea a parsear\n");
			linea= solicitarBytesUMC(umc,numPag,offset,tamanio);
			//Parseo Linea-------------------------------------------------
			parsearLinea((char*) linea);
			usleep((int)quantum_sleep);
			if(flagSaltoLinea==1){
				flagSaltoLinea=0;
			}
			else{
				pcb->PC++;
			}
			notificarNucleoQuantum(nucleo);
		}

	}

		finalizarCPU();


	config_destroy(cfgCPU);
	return 0;
}
