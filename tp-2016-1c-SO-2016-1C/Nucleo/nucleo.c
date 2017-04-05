#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <commons/config.h>
#include <semaphore.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <sockets.h>
#include <estructuras.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <signal.h>

typedef struct{
	char* nomSem;
	int valor;
	t_queue* colSem;
}t_semAnsisop;

typedef struct{
	char* nomIO;
	int retardo;
	t_queue* colIO;
	sem_t semIO;
	pthread_mutex_t mutexIO;
}t_IO;

t_config* cfgNUCLEO;
int UMC; //file descriptor de la UMC
pthread_mutex_t mutexReady;
pthread_mutex_t mutexCPUs;
pthread_mutex_t mutexQsleep;
sem_t semReady;
sem_t semCpu;
uint32_t quantum;
uint32_t qSleep;
int stackSize;
uint32_t tamPag;
t_dictionary* varGlobales;
t_queue* readyPCBs;
t_list* listaCPUs;
//t_list* listaExit; no se si usarla porque hice todo para que apenas tenga que eliminar algo lo haga, sin tener agregarlo a una lista
t_list* listaNew;
t_list* lIO;
t_list* semaforos;
int iNotify;
fd_set master;   // conjunto maestro de descriptores de fichero
void *buf;    // buffer para datos del cliente

void mandarAImprimirTexto(int s_cpu,uint32_t tamanioTexto){
	void *bufAConsola=malloc(sizeof(uint32_t)+tamanioTexto);
	memcpy(bufAConsola,&tamanioTexto,sizeof(uint32_t));
	recv(s_cpu,bufAConsola+sizeof(uint32_t),tamanioTexto,0);
	uint32_t pID;
	recv(s_cpu,&pID,sizeof(uint32_t),0);
	send(pID,bufAConsola,sizeof(uint32_t)+tamanioTexto,0);
	free(bufAConsola);
}

void mandarAImprimir(int s_cpu,uint32_t pID){
	uint32_t valorAImprimir;
	recv(s_cpu,&valorAImprimir,sizeof(uint32_t),0);
	int orden=IMPRIMIR;
	void * msjConsola=malloc(sizeof(uint32_t)+sizeof(int));
	memcpy(msjConsola,&orden,sizeof(int));
	memcpy(msjConsola+sizeof(int),&valorAImprimir,sizeof(u_int32_t));
	send((int)pID,msjConsola,(sizeof(uint32_t)+sizeof(int)),0);
	free(msjConsola);
}

void serializarPCByEnviar(t_PCB* pcb, int s_cpu){

	uint32_t accion = ENVIO_PCB;

	void* buffer = malloc(6000);
	uint32_t tamanioValidoBuffer = 0;

	//Copio Accion
	memcpy(buffer + tamanioValidoBuffer, &accion, sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio Quantum Sleep
	pthread_mutex_lock(&mutexQsleep);
	memcpy(buffer + tamanioValidoBuffer, &qSleep, sizeof(uint32_t));
	pthread_mutex_unlock(&mutexQsleep);
	tamanioValidoBuffer += sizeof(uint32_t);

	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio PID
	memcpy(buffer + tamanioValidoBuffer, &pcb->PID, sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio PC
	memcpy(buffer + tamanioValidoBuffer, &pcb->PC, sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio Cant Paginas Codigo
	memcpy(buffer + tamanioValidoBuffer, &pcb->paginasCodigo,
			sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio Cant Paginas Stack
	memcpy(buffer + tamanioValidoBuffer, &pcb->paginasStack,
			sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio Cant de Operaciones de IO
	memcpy(buffer + tamanioValidoBuffer, &pcb->cantOperacIO,
			sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio Tamanio Indice de Codigo
	memcpy(buffer + tamanioValidoBuffer, &pcb->tamanioIndiceDeCodigo,
			sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio Indice de Codigo
	int i=0;
	while(i<pcb->tamanioIndiceDeCodigo){
		memcpy(buffer+tamanioValidoBuffer,&pcb->indiceDeCodigo[i].start,sizeof(uint32_t));
		tamanioValidoBuffer += sizeof(uint32_t);
		memcpy(buffer+tamanioValidoBuffer,&pcb->indiceDeCodigo[i].offset,sizeof(uint32_t));
		tamanioValidoBuffer += sizeof(uint32_t);
		i++;
	}

	/*memcpy(buffer + tamanioValidoBuffer, pcb->indiceDeCodigo,
			pcb->tamanioIndiceDeCodigo);
	tamanioValidoBuffer += pcb->tamanioIndiceDeCodigo;
*/
	//Copio Etiquetas Size
	memcpy(buffer + tamanioValidoBuffer,
			&pcb->indiceDeEtiquetas.etiquetas_size, sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio Etiquetas
	memcpy(buffer + tamanioValidoBuffer, pcb->indiceDeEtiquetas.etiquetas,
			pcb->indiceDeEtiquetas.etiquetas_size);
	tamanioValidoBuffer += pcb->indiceDeEtiquetas.etiquetas_size;

	//Copio SP
	memcpy(buffer + tamanioValidoBuffer, &pcb->SP, sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio Tamanio Indice de Stack
	memcpy(buffer + tamanioValidoBuffer, &pcb->tamanioIndiceStack,
			sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	if(pcb->tamanioIndiceStack>0){
		int i=0;
		while(i<list_size(pcb->indiceDeStack)){
			t_stack* stack=malloc(sizeof(t_stack));
			stack=list_remove(pcb->indiceDeStack,i);
			memcpy(buffer + tamanioValidoBuffer, &stack->cantArgs,
					sizeof(uint32_t));
			tamanioValidoBuffer += sizeof(uint32_t);
			if(stack->cantArgs>0){
				int j=0;
				while(j<list_size(stack->args)){
					t_variable* arg=malloc(sizeof(t_variable));
					arg=list_remove(stack->args,j);
					memcpy(buffer + tamanioValidoBuffer, &arg->ID,
										sizeof(char));
					tamanioValidoBuffer += sizeof(char);
					memcpy(buffer + tamanioValidoBuffer, &arg->posicionEnMemoria.pag,
										sizeof(uint32_t));
					tamanioValidoBuffer += sizeof(uint32_t);
					memcpy(buffer + tamanioValidoBuffer, &arg->posicionEnMemoria.offset,sizeof(uint32_t));
					tamanioValidoBuffer += sizeof(uint32_t);
					j++;
				}
			}
			memcpy(buffer + tamanioValidoBuffer, &stack->cantVars,
					sizeof(uint32_t));
			tamanioValidoBuffer += sizeof(uint32_t);
			if(stack->cantVars>0){
				int j=0;
				while(j<list_size(stack->vars)){
					t_variable* var=malloc(sizeof(t_variable));
					var=list_remove(stack->vars,j);
					memcpy(buffer + tamanioValidoBuffer, &var->ID,
										sizeof(char));
					tamanioValidoBuffer += sizeof(char);
					memcpy(buffer + tamanioValidoBuffer, &var->posicionEnMemoria.pag,
										sizeof(uint32_t));
					tamanioValidoBuffer += sizeof(uint32_t);
					memcpy(buffer + tamanioValidoBuffer, &var->posicionEnMemoria.offset,sizeof(uint32_t));
					tamanioValidoBuffer += sizeof(uint32_t);
					j++;
				}
			}
		memcpy(buffer + tamanioValidoBuffer, &stack->retPos,
				sizeof(uint32_t));
		tamanioValidoBuffer += sizeof(uint32_t);
		memcpy(buffer + tamanioValidoBuffer, &stack->retVar.pag,
				sizeof(uint32_t));
		tamanioValidoBuffer += sizeof(uint32_t);
		memcpy(buffer + tamanioValidoBuffer, &stack->retVar.offset,
				sizeof(uint32_t));
		tamanioValidoBuffer += sizeof(uint32_t);
		i++;
		}
	}
	uint32_t tamanioPCB= tamanioValidoBuffer-(sizeof(uint32_t)*2);
	///Envio PCB a la CPU
	memcpy(buffer+(sizeof(uint32_t)*2),&tamanioPCB,sizeof(uint32_t));
	send(s_cpu, buffer, tamanioValidoBuffer, 0);
	printf("mande el pcb al CPU\n");
	free(buffer);
}

t_PCB* deserializarPCB(int s_cpu){
	uint32_t tam_pcb;
	recv(s_cpu,&tam_pcb,sizeof(uint32_t),0);
	t_PCB* pcb=malloc(tam_pcb);
	//aca tengo que deserializar lo que recibi de la CPU y armar el pcb

	//Recibo PID
	int bytesRecibidos = recv(s_cpu, &(pcb->PID), sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó3.");
	}

	//Recibo PC
	bytesRecibidos = recv(s_cpu, &(pcb->PC), sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó4.");
	}

	//Recibo Cantidad de Paginas de Codigo
	bytesRecibidos = recv(s_cpu, &(pcb->paginasCodigo),
			sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó5.");
	}

	//Recibo Cantidad de Paginas de Stack
	bytesRecibidos = recv(s_cpu, &(pcb->paginasStack),
			sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó5.");
	}

	//Recibo Cantidad de Operaciones de IO
	bytesRecibidos = recv(s_cpu, &(pcb->cantOperacIO),
			sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó5.");
	}

	//Recibo Tamanio Indice de Codigo
	bytesRecibidos = recv(s_cpu, &(pcb->tamanioIndiceDeCodigo),
			sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó6.");
	}

	//Recibo Indice de Codigo
	pcb->indiceDeCodigo = malloc(pcb->tamanioIndiceDeCodigo*sizeof(t_intructions));
	int i=0;
	while(i<pcb->tamanioIndiceDeCodigo){
		recv(s_cpu, &(pcb->indiceDeCodigo[i].start),sizeof(uint32_t), 0);
		recv(s_cpu, &(pcb->indiceDeCodigo[i].offset),sizeof(uint32_t), 0);
		i++;
	}
	/*pcb->indiceDeCodigo = malloc(pcb->tamanioIndiceDeCodigo);

	bytesRecibidos = recv(s_cpu, pcb->indiceDeCodigo,
			pcb->tamanioIndiceDeCodigo, 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó7.");
	}*/

	//Recibo Tamanio Indice de Etiquetas

	bytesRecibidos = recv(s_cpu,
			&(pcb->indiceDeEtiquetas.etiquetas_size), sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó8.");
	}

	if (pcb->indiceDeEtiquetas.etiquetas_size > 0) {
		//Recibo Indice de Etiquetas
		pcb->indiceDeEtiquetas.etiquetas = malloc(
				pcb->indiceDeEtiquetas.etiquetas_size);

		bytesRecibidos = recv(s_cpu, pcb->indiceDeEtiquetas.etiquetas,
				pcb->indiceDeEtiquetas.etiquetas_size, 0);
		if (bytesRecibidos <= 0) {
			perror("El emisor se desconectó9.");
		}
	}
	//Recibo Stack Pointer

	bytesRecibidos = recv(s_cpu, &(pcb->SP), sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó10.");
	}

	//Recibo Tamanio Indice de Stack

	bytesRecibidos = recv(s_cpu, &(pcb->tamanioIndiceStack),
			sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó11.");
	}

	if (pcb->tamanioIndiceStack > 0) {
		//Recibo Indice de Stack
		pcb->indiceDeStack = list_create();
		int i=0;
		while(i<pcb->tamanioIndiceStack){
			t_stack* stackAux=malloc(sizeof(t_stack));
			recv(s_cpu,&stackAux->cantArgs,sizeof(uint32_t),0);
			if(stackAux->cantArgs>0){
				recv(s_cpu,stackAux->args,sizeof(t_list),0);
				stackAux->args=list_create();
				int j=0;
				while(j<(stackAux->cantArgs)){
					t_variable* argAux=malloc(sizeof(t_variable));
					recv(s_cpu,&argAux->ID,sizeof(char),0);
					//recv(s_cpu,&argAux->posicionEnMemoria,sizeof(t_posMemoria),0);
					recv(s_cpu,&argAux->posicionEnMemoria.pag,sizeof(uint32_t),0);
					recv(s_cpu,&argAux->posicionEnMemoria.offset,sizeof(uint32_t),0);
					list_add(stackAux->args,argAux);
					j++;
				}
			}
			recv(s_cpu,&stackAux->cantVars,sizeof(uint32_t),0);
			if(stackAux->cantVars>0){
				recv(s_cpu,stackAux->vars,sizeof(t_list),0);
				stackAux->vars=list_create();
				int j=0;
				while(j<(stackAux->cantVars)){
					t_variable* varAux=malloc(sizeof(t_variable));
					recv(s_cpu,&varAux->ID,sizeof(char),0);
					//recv(s_cpu,&varAux->posicionEnMemoria,sizeof(t_posMemoria),0);
					recv(s_cpu,&varAux->posicionEnMemoria.pag,sizeof(uint32_t),0);
					recv(s_cpu,&varAux->posicionEnMemoria.offset,sizeof(uint32_t),0);
					list_add(stackAux->args,varAux);
					j++;
				}
			}
			recv(s_cpu,&stackAux->retPos,sizeof(uint32_t),0);
			recv(s_cpu,&stackAux->retVar.pag,sizeof(uint32_t),0);
			recv(s_cpu,&stackAux->retVar.offset,sizeof(uint32_t),0);
			list_add(pcb->indiceDeStack,stackAux);
			i++;
		}
	}
	return pcb;
}

t_PCB* deserializarPCBTradicional(int s_cpu, uint32_t tam_pcb){
	t_PCB* pcb=malloc(tam_pcb);
	//aca tengo que deserializar lo que recibi de la CPU y armar el pcb

	//Recibo PID
	int bytesRecibidos = recv(s_cpu, &(pcb->PID), sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó3.");
	}

	//Recibo PC
	bytesRecibidos = recv(s_cpu, &(pcb->PC), sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó4.");
	}

	//Recibo Cantidad de Paginas de Codigo
	bytesRecibidos = recv(s_cpu, &(pcb->paginasCodigo),
			sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó5.");
	}

	//Recibo Cantidad de Paginas de Stack
	bytesRecibidos = recv(s_cpu, &(pcb->paginasStack),
			sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó5.");
	}

	//Recibo Cantidad de Operaciones de IO
	bytesRecibidos = recv(s_cpu, &(pcb->cantOperacIO),
			sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó5.");
	}

	//Recibo Tamanio Indice de Codigo
	bytesRecibidos = recv(s_cpu, &(pcb->tamanioIndiceDeCodigo),
			sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó6.");
	}

	//Recibo Indice de Codigo
	pcb->indiceDeCodigo = malloc(pcb->tamanioIndiceDeCodigo*sizeof(t_intructions));
	int i=0;
	while(i<pcb->tamanioIndiceDeCodigo){
		recv(s_cpu, &(pcb->indiceDeCodigo[i].start),sizeof(uint32_t), 0);
		recv(s_cpu, &(pcb->indiceDeCodigo[i].offset),sizeof(uint32_t), 0);
		i++;
	}
	/*pcb->indiceDeCodigo = malloc(pcb->tamanioIndiceDeCodigo);

	bytesRecibidos = recv(s_cpu, pcb->indiceDeCodigo,
			pcb->tamanioIndiceDeCodigo, 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó7.");
	}*/

	//Recibo Tamanio Indice de Etiquetas

	bytesRecibidos = recv(s_cpu,
			&(pcb->indiceDeEtiquetas.etiquetas_size), sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó8.");
	}

	if (pcb->indiceDeEtiquetas.etiquetas_size > 0) {
		//Recibo Indice de Etiquetas
		pcb->indiceDeEtiquetas.etiquetas = malloc(
				pcb->indiceDeEtiquetas.etiquetas_size);

		bytesRecibidos = recv(s_cpu, pcb->indiceDeEtiquetas.etiquetas,
				pcb->indiceDeEtiquetas.etiquetas_size, 0);
		if (bytesRecibidos <= 0) {
			perror("El emisor se desconectó9.");
		}
	}
	//Recibo Stack Pointer

	bytesRecibidos = recv(s_cpu, &(pcb->SP), sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó10.");
	}

	//Recibo Tamanio Indice de Stack

	bytesRecibidos = recv(s_cpu, &(pcb->tamanioIndiceStack),
			sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		perror("El emisor se desconectó11.");
	}

	if (pcb->tamanioIndiceStack > 0) {
		//Recibo Indice de Stack
		pcb->indiceDeStack = list_create();
		int i=0;
		while(i<pcb->tamanioIndiceStack){
			t_stack* stackAux=malloc(sizeof(t_stack));
			recv(s_cpu,&stackAux->cantArgs,sizeof(uint32_t),0);
			if(stackAux->cantArgs>0){
				recv(s_cpu,stackAux->args,sizeof(t_list),0);
				stackAux->args=list_create();
				int j=0;
				while(j<(stackAux->cantArgs)){
					t_variable* argAux=malloc(sizeof(t_variable));
					recv(s_cpu,&argAux->ID,sizeof(char),0);
					//recv(s_cpu,&argAux->posicionEnMemoria,sizeof(t_posMemoria),0);
					recv(s_cpu,&argAux->posicionEnMemoria.pag,sizeof(uint32_t),0);
					recv(s_cpu,&argAux->posicionEnMemoria.offset,sizeof(uint32_t),0);
					list_add(stackAux->args,argAux);
					j++;
				}
			}
			recv(s_cpu,&stackAux->cantVars,sizeof(uint32_t),0);
			if(stackAux->cantVars>0){
				recv(s_cpu,stackAux->vars,sizeof(t_list),0);
				stackAux->vars=list_create();
				int j=0;
				while(j<(stackAux->cantVars)){
					t_variable* varAux=malloc(sizeof(t_variable));
					recv(s_cpu,&varAux->ID,sizeof(char),0);
					//recv(s_cpu,&varAux->posicionEnMemoria,sizeof(t_posMemoria),0);
					recv(s_cpu,&varAux->posicionEnMemoria.pag,sizeof(uint32_t),0);
					recv(s_cpu,&varAux->posicionEnMemoria.offset,sizeof(uint32_t),0);
					list_add(stackAux->args,varAux);
					j++;
				}
			}
			recv(s_cpu,&stackAux->retPos,sizeof(uint32_t),0);
			recv(s_cpu,&stackAux->retVar.pag,sizeof(uint32_t),0);
			recv(s_cpu,&stackAux->retVar.offset,sizeof(uint32_t),0);
			list_add(pcb->indiceDeStack,stackAux);
			i++;
			}
		}
	return pcb;
}

void mandarAready(int s_cpu, uint32_t tam_pcb){
	t_PCB* pcb= deserializarPCBTradicional(s_cpu,tam_pcb);
	pthread_mutex_lock(&mutexReady);
	queue_push(readyPCBs,pcb);
	pthread_mutex_unlock(&mutexReady);
}

t_semAnsisop* obtenerNodoSemaforo(char* sID){
	int i=0;
	int acierto=0;
	t_semAnsisop* semaforo;
		while(i<list_size(semaforos)&&acierto==0){
			semaforo=list_get(semaforos,i);
			if(strcmp(semaforo->nomSem,sID)==0){
				acierto=1;
				}
			i++;
		}
	return semaforo;
}

t_IO* obtenerNodoIO(char*sID){
	int i=0;
	int acierto=0;
	t_IO* iO;
	while(i<list_size(lIO)&&acierto==0){
		iO=list_get(semaforos,i);
		if(strcmp(iO->nomIO,sID)==0){
			acierto=1;
		}
		i++;
	}
	return iO;
}

void actualizarQuantumCPU(int s_cpu){
	int i=0;
	int acierto=0;
	t_CPU* cpuAux=malloc(sizeof(t_CPU*));
	uint32_t orden;
	//wait mutex lista CPUs
	pthread_mutex_lock(&mutexCPUs);
	while((i<list_size(listaCPUs))&&(acierto==0)){
		cpuAux=list_get(listaCPUs,i);
		if(cpuAux->s_cpu==(uint32_t)s_cpu){
			acierto=1;
			cpuAux->q_asoc-=1;
			if(cpuAux->q_asoc==0){
				orden=DEVOLVER_PCB;
				send(s_cpu,&orden,sizeof(uint32_t),0);
			}
			else{
				orden=SEGUIR_EJECUTANDO;
				send(s_cpu,&orden,sizeof(uint32_t),0);
			}
		}
		i++;
	}
	pthread_mutex_unlock(&mutexCPUs);
	//signal Mutex lista CPUs
}

int actualizarCPUlibre(int cpu){
	int i=0;
	int acierto=0;
	int valor;
	//wait mutex listaCPUs
	pthread_mutex_lock(&mutexCPUs);
	while((i<list_size(listaCPUs))&&(acierto==0)){
		t_CPU* cpuAux=list_get(listaCPUs,i);
		if(((int)cpuAux->s_cpu)==cpu){
			acierto=1;
			cpuAux->estado=1;
			valor=cpuAux->estadoPCB;
			cpuAux->estadoPCB=0;
			cpuAux->q_asoc=quantum;
			cpuAux->pcbAsoc=0;
		}
		i++;
	}
	pthread_mutex_unlock(&mutexCPUs);
	//signal mutex listaCPUs
	return valor;
}

void bloquearPCBaSemaforo(int s_cpu, uint32_t tambuf){
		char* ID=malloc(sizeof(tambuf));
		recv(s_cpu,ID,tambuf,0);
		t_semAnsisop* sem=obtenerNodoSemaforo(ID);
		t_PCB* pcb=deserializarPCB(s_cpu);
		queue_push(sem->colSem,pcb);
}

void bloquearPCBaIO(int s_cpu, uint32_t tambuf){
		char* ID=malloc(sizeof(tambuf));
		recv(s_cpu,ID,tambuf,0);
		t_IO* nIO=obtenerNodoIO(ID);
		t_PCB* pcb=deserializarPCB(s_cpu);
		pthread_mutex_lock(&nIO->mutexIO);
		queue_push(nIO->colIO,pcb);
		pthread_mutex_unlock(&nIO->mutexIO);
		sem_post(&nIO->semIO);
}

void inicializarVarGlobales(){
	uint32_t* valInitVar=malloc(sizeof(uint32_t));
	*valInitVar=0;
	varGlobales=dictionary_create();
	char** varAInsertar=config_get_array_value(cfgNUCLEO,"SHARED_VARS");
	int i=0;
	while(varAInsertar[i]!=NULL){
		dictionary_put(varGlobales,varAInsertar[i],valInitVar);
		i++;
	}
	free(valInitVar);
	printf("Se crearon las variables globales\n");
}

void inicializarSemaforos(){
	semaforos=list_create();
	char** varAInsertar=config_get_array_value(cfgNUCLEO,"SEM_IDS");
	char** valVar=config_get_array_value(cfgNUCLEO,"SEM_INIT");
	int i=0;
	while(varAInsertar[i]!=NULL){
		t_semAnsisop* sem=malloc(sizeof(t_semAnsisop));
		sem->nomSem=varAInsertar[i];
		sem->valor=atoi(valVar[i]);
		sem->colSem=queue_create();
		list_add(semaforos,sem);
		i++;
	}
	printf("Se crearon los semáforos\n");
}

void inicializarEntradaSalida(){
	lIO=list_create();
	char** varAInsertar=config_get_array_value(cfgNUCLEO,"IO_IDS");
	char** valVar=config_get_array_value(cfgNUCLEO,"IO_SLEEP");
	int i=0;
	while(varAInsertar[i]!=NULL){
		t_IO* newIO=malloc(sizeof(t_IO));
		newIO->nomIO=varAInsertar[i];
		newIO->retardo=atoi(valVar[i]);
		newIO->colIO=queue_create();
		pthread_mutex_init(&newIO->mutexIO,NULL);
		sem_init(&newIO->semIO,0,0);
		list_add(lIO,newIO);
		i++;
	}
	printf("Se crearon los dispositivos de Entrada/Salida\n");
}

void inicializarEstructuras(){
	quantum=config_get_int_value(cfgNUCLEO,"QUANTUM");
	printf("%d\n",quantum);
	qSleep=config_get_int_value(cfgNUCLEO,"QUANTUM_SLEEP");
	stackSize=config_get_int_value(cfgNUCLEO,"STACK_SIZE");
	inicializarVarGlobales();
	inicializarEntradaSalida();
	inicializarSemaforos();
	readyPCBs=queue_create();
	listaCPUs=list_create();
//	listaExit=list_create();
//	listaNew=list_create();
}

int obtenerCantPags(char*codigoAnsisop){
	int resto=(strlen(codigoAnsisop)+1)%tamPag;
	if(resto==0){
		return (strlen(codigoAnsisop)+1)/tamPag;
	}
	return ((strlen(codigoAnsisop)+1)/tamPag)+1;
}

uint32_t solicitarPaginasUMC(int pID,char* programa){
	uint32_t resultado;
	uint32_t procID=pID;
	uint32_t cantPag=obtenerCantPags(programa);
	uint32_t tamPrograma=strlen(programa)+1;
	uint32_t orden=INICIALIZAR_PROGRAMA_UMC;
	int puntero=0;
	void* buffer=malloc(sizeof(uint32_t)*4+tamPrograma);
	memcpy(buffer,&orden,sizeof(uint32_t));
	puntero+=sizeof(uint32_t);
	memcpy(buffer+puntero,&procID,sizeof(uint32_t));
	puntero+=sizeof(uint32_t);
	memcpy(buffer+puntero,&cantPag,sizeof(uint32_t));
	puntero+=sizeof(uint32_t);
	memcpy(buffer+puntero,&tamPrograma,sizeof(uint32_t));
	puntero+=sizeof(uint32_t);
	memcpy(buffer+puntero,programa,tamPrograma);
	puntero+=tamPrograma;
	send(UMC,buffer,puntero,0);
	recv(UMC,&resultado,sizeof(uint32_t),0);
	free(buffer);
	return resultado;
}

void inicializarPCB(int pID,char* codigoAnsisop) {
	u_int32_t resp=solicitarPaginasUMC(pID,codigoAnsisop);
	if((int)resp==PROGRAMA_INICIALIZADO){
		t_PCB* pcb=malloc(sizeof(t_PCB));
		t_metadata_program* metaProg=metadata_desde_literal(codigoAnsisop);
		pcb->PID=(uint32_t)pID;
		pcb->PC=metaProg->instruccion_inicio;
		pcb->indiceDeCodigo=metaProg->instrucciones_serializado;
		pcb->tamanioIndiceDeCodigo=metaProg->instrucciones_size;
		pcb->indiceDeEtiquetas.etiquetas=metaProg->etiquetas;
		pcb->indiceDeEtiquetas.etiquetas_size=metaProg->etiquetas_size;
		pcb->paginasCodigo=(obtenerCantPags(codigoAnsisop));
		pcb->paginasStack=stackSize;
		pcb->cantOperacIO=0;
		pcb->SP=0;
		pcb->tamanioIndiceStack=0;
		pthread_mutex_lock(&mutexReady);
		queue_push(readyPCBs,pcb);
		pthread_mutex_unlock(&mutexReady);
		sem_post(&semReady);
		free(metaProg);
	}
	else if((int)resp==PROGRAMA_NO_INICIALIZADO){
		send(pID,&resp,sizeof(uint32_t),0);
		close(pID);
	}
}

void inicializarAnsisop(int s_consola, uint32_t cant_leer){
	char* progAnsisop=malloc(cant_leer);
	recv(s_consola,progAnsisop,cant_leer,0);
	printf("%s\n",progAnsisop);
	inicializarPCB(s_consola,progAnsisop);
	free(progAnsisop);
}

void obtenerValorGlobal(int s_cpu, uint32_t tam_buf){
	char* varGlob=malloc(tam_buf);
	recv(s_cpu,varGlob,tam_buf,0);
	uint32_t* valor=dictionary_get(varGlobales,varGlob);
	printf("esta variable tengo que mandar a la CPU: %d\n",(int)valor);
	uint32_t orden=VALOR_GLOBAL;
	void* buffer=malloc(sizeof(uint32_t)*2);
	memcpy(buffer,&orden,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t),valor,sizeof(uint32_t));
	send(s_cpu,buffer,(sizeof(uint32_t)*2),0);
}

void grabarValor(int s_cpu, uint32_t tam_buf){
	uint32_t valor;
	recv(s_cpu,&valor,sizeof(uint32_t),0);
	char* varGlob=malloc(tam_buf-sizeof(uint32_t));
	recv(s_cpu,varGlob,(tam_buf-sizeof(uint32_t)),0);
	dictionary_put(varGlobales,varGlob,&valor);
	printf("actualice el valor de la variable global: %s a: %d\n",varGlob,(int)valor);
}

void decrementarSemaforo(int s_cpu, char* id){
	int i=0;
	int acierto=0;
	uint32_t orden;
	while(i<list_size(semaforos)&&acierto==0){
		t_semAnsisop* sem=list_get(semaforos,i);
		if(strcmp(sem->nomSem,id)==0){
			acierto=1;
			sem->valor-=1;
			if(sem->valor<0){
				orden=BLOQUEADO;
				send(s_cpu,&orden,sizeof(uint32_t),0);
			}
			else{
				orden=NO_BLOQUEADO;
				send(s_cpu,&orden,sizeof(uint32_t),0);
			}
		}
		i++;
	}
}

void aumentarSemaforo(char* id){
	int i=0;
	int acierto=0;
	while(i<list_size(semaforos)&&acierto==0){
		t_semAnsisop* sem=list_get(semaforos,i);
		if(strcmp(sem->nomSem,id)==0){
			acierto=1;
			sem->valor+=1;
			if(!queue_is_empty(sem->colSem)){
				t_PCB* pcb=queue_pop(sem->colSem);
					pthread_mutex_lock(&mutexReady);
					queue_push(readyPCBs,pcb);
					pthread_mutex_unlock(&mutexReady);
				}
			}
		i++;
		}
}

void waitSemaforo(int s_cpu, uint32_t tam_buf){
	char* semID=malloc(tam_buf);
	recv(s_cpu,semID,tam_buf,0);
	decrementarSemaforo(s_cpu,semID);
	free(semID);
}

void signalSemaforo(int s_cpu, uint32_t tam_buf){
	char* semID=malloc(tam_buf);
	recv(s_cpu,semID,tam_buf,0);
	aumentarSemaforo(semID);
	free(semID);
}

void finalizarPCBTradicional(int s_cpu,uint32_t tam_pcb){
	t_PCB* pcb=deserializarPCBTradicional(s_cpu,tam_pcb);
	printf("deserialice bien piola\n");
	uint32_t ordenUMC=FINALIZAR_PROGRAMA_UMC;
	uint32_t ordenConsola=PROGRAMA_FINALIZO;
	void* buffer=malloc(sizeof(uint32_t)*2);
	memcpy(buffer,&ordenUMC,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t),&pcb->PID,sizeof(uint32_t));
	send(UMC,buffer,sizeof(uint32_t)*2,0);
	send((int)pcb->PID,&ordenConsola,sizeof(uint32_t),0);
	printf("mande a la consola que el ansisop finalizo\n");
	close((int)pcb->PID);
	FD_CLR((int)pcb->PID,&master);
	free(pcb);
}

void finalizarPCB(int s_cpu){
	t_PCB* pcb=deserializarPCB(s_cpu);
	uint32_t ordenUMC=FINALIZAR_PROGRAMA_UMC;
	uint32_t ordenConsola=PROGRAMA_FINALIZO;
	void* buffer=malloc(sizeof(uint32_t)*2);
	memcpy(buffer,&ordenUMC,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t),&pcb->PID,sizeof(uint32_t));
	send(UMC,buffer,sizeof(uint32_t)*2,0);
	send((int)pcb->PID,&ordenConsola,sizeof(uint32_t),0);
	close((int)pcb->PID);
	FD_CLR((int)pcb->PID,&master);
	free(pcb);
}

void buscarPCByEliminar(int pID){
	int i=0;
	int j;
	int acierto=0;
	t_PCB* pcb=malloc(sizeof(t_PCB));
	t_CPU* cpu=malloc(sizeof(t_CPU));
	t_semAnsisop* semAux=malloc(sizeof(t_semAnsisop));
	t_IO* ioAux= malloc(sizeof(t_IO));
	i=0;
	pthread_mutex_lock(&mutexCPUs);
	while(i<list_size(listaCPUs)&&acierto==0){
		cpu=list_get(listaCPUs,i);
		if(cpu->pcbAsoc==(uint32_t)pID){
			acierto=1;
			cpu->estadoPCB=1;
		}
		i++;
	}
	pthread_mutex_unlock(&mutexCPUs);
	i=0;
	pthread_mutex_lock(&mutexReady);
	while(i<queue_size(readyPCBs)&&acierto==0){
		pcb=queue_pop(readyPCBs);
		if(pcb->PID==(uint32_t)pID){
			acierto=1;
			if(i!=0){
				int iAux=i;
				t_PCB* pcbAux=malloc(sizeof(t_PCB));
				while(iAux<queue_size(readyPCBs)){
					pcbAux=queue_pop(readyPCBs);
					queue_push(readyPCBs,pcbAux);
				}
			}
		}
		else{
		queue_push(readyPCBs,pcb);
		}
		i++;
	}
	pthread_mutex_unlock(&mutexReady);
	i=0;
	j=0;
	while(i<list_size(semaforos)&&acierto==0){
		semAux=list_get(semaforos,i);
		while(j<queue_size(semAux->colSem)){
			pcb=queue_pop(semAux->colSem);
			if(pcb->PID==(uint32_t)pID){
				acierto=1;
				if(j!=0){
					int jAux=j;
					t_PCB* pcbAux=malloc(sizeof(t_PCB));
					while(jAux<queue_size(semAux->colSem)){
						pcbAux=queue_pop(semAux->colSem);
						queue_push(semAux->colSem,pcbAux);
					}
				}
			}
			else{
			queue_push(semAux->colSem,pcb);
			}
			j++;
		}
		i++;
	}
	i=j=0;
	while((i<list_size(lIO))&&(acierto==0)){
		ioAux=list_get(lIO,i);
		//wait mutex cola IO
		pthread_mutex_lock(&ioAux->mutexIO);
		while(j<queue_size(ioAux->colIO)){
			pcb=queue_pop(semAux->colSem);
				if(pcb->PID==(uint32_t)pID){
					acierto=1;
					if(j!=0){
						int jAux=j;
						t_PCB* pcbAux=malloc(sizeof(t_PCB));
						while(jAux<queue_size(semAux->colSem)){
							pcbAux=queue_pop(semAux->colSem);
							queue_push(semAux->colSem,pcbAux);
						}
					}
				//wait semaforo contador pcbs en cola IO
				sem_wait(&ioAux->semIO);
				}
				else{
					queue_push(semAux->colSem,pcb);
				}
			j++;
		}
		pthread_mutex_unlock(&ioAux->mutexIO);
		//signal mutex cola IO
		i++;
	}
	uint32_t orden=FINALIZAR_PROGRAMA_UMC;
	void* buffer=malloc(sizeof(uint32_t)*2);
	memcpy(buffer,&orden,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t),&pcb->PID,sizeof(uint32_t));
	send(UMC,buffer,sizeof(uint32_t)*2,0);
	free(pcb);
	free(cpu);
	free(semAux);
	free(ioAux);
}

void eliminarDatosNoRelevantesSem(int s_cpu, uint32_t tamanio){
	uint32_t tamanioAux=0;
	char* id=malloc(tamanio);
	recv(s_cpu,id,tamanio,0);
	tamanioAux+=tamanio;
	free(id);
}

void eliminarDatosNoRelevantesIO(int s_cpu,uint32_t tamanio){
	uint32_t tamanioAux=0;
	char* id=malloc(tamanio);
	uint32_t cantOperaciones;
	recv(s_cpu,id,tamanio,0);
	tamanioAux+=tamanio;
	recv(s_cpu,&cantOperaciones,sizeof(uint32_t),0);
	tamanioAux+=sizeof(uint32_t);
	free(id);
}

void eliminarCPU(int cpuID){
	int i=0;
	int acierto=0;
	t_CPU* cpuAeliminar;
	pthread_mutex_lock(&mutexCPUs);
	while(i<list_size(listaCPUs)&&acierto==0){
		cpuAeliminar=list_get(listaCPUs,i);
		if(cpuAeliminar->s_cpu==cpuID){
			acierto=1;
			cpuAeliminar=list_remove(listaCPUs,i);
		}
		i++;
	}
	pthread_mutex_unlock(&mutexCPUs);
	free(cpuAeliminar);
	close(cpuID);
	FD_CLR(cpuID,&master);
	sem_post(&semCpu);
}

void atenderOrdenSegunID(int socket,uint32_t id, int tamanio) {
	uint32_t orden;
	uint32_t tamanio_buffer;
	int mov_puntero=tamanio;
	int valor;
	switch ((int) id) {
		case ID_CONSOLA:
			memcpy(&orden, buf + mov_puntero, sizeof(uint32_t));
			mov_puntero += sizeof(uint32_t);
			switch ((int) orden) {
				case INICIALIZAR_ANSISOP: {
					memcpy(&tamanio_buffer, buf + mov_puntero, sizeof(uint32_t));
					inicializarAnsisop(socket,tamanio_buffer); //aca inicializo el programa Ansisop, osea PCB etc etc
				}
				break;
				case FINALIZAR_ANSISOP: {
					memcpy(&tamanio_buffer, buf + mov_puntero, sizeof(uint32_t));
					buscarPCByEliminar(socket);
					close(socket);
					FD_CLR(socket,&master);
				}
			}
		break;
		case ID_CPU:
			memcpy(&orden, buf + mov_puntero, sizeof(uint32_t));
			mov_puntero += sizeof(uint32_t);
			switch((int)orden){
				case OBTENER_VALOR:
					memcpy(&tamanio_buffer, buf + mov_puntero, sizeof(uint32_t));
					obtenerValorGlobal(socket,tamanio_buffer);
				break;
				case GRABAR_VALOR:
					memcpy(&tamanio_buffer, buf + mov_puntero, sizeof(uint32_t));
					grabarValor(socket,tamanio_buffer);
				break;
				case WAIT:
					memcpy(&tamanio_buffer, buf + mov_puntero, sizeof(uint32_t));
					waitSemaforo(socket,tamanio_buffer);
				break;
				case SIGNAL:
					memcpy(&tamanio_buffer, buf + mov_puntero, sizeof(uint32_t));
					signalSemaforo(socket,tamanio_buffer);
				break;
				case ENTRADA_SALIDA:
					memcpy(&tamanio_buffer, buf + mov_puntero, sizeof(uint32_t));
					valor=actualizarCPUlibre(socket);
					if(valor==0){
					bloquearPCBaIO(socket,tamanio_buffer);
					}
					else{
					eliminarDatosNoRelevantesIO(socket,tamanio_buffer);
					finalizarPCB(socket);
					//en el metodo finalizarPCB debería obtener el ID del proceso y hacer send a la UMC para que libere la memoria asociada al mismo
					}
				break;
				case PASO_QUANTUM:
					memcpy(&tamanio_buffer, buf + mov_puntero, sizeof(uint32_t));
					actualizarQuantumCPU(socket);
				break;
				case FIN_ANSISOP:
					printf("vamo a finalizar el Ansisop\n");
					memcpy(&tamanio_buffer, buf + mov_puntero, sizeof(uint32_t));
					finalizarPCBTradicional(socket,tamanio_buffer);
					//aca debería obtener el ID del proceso y hacer send a la UMC para que libere la memoria asociada al mismo
				break;
				case BLOQUEAR_PCB_SEMAFORO:
					memcpy(&tamanio_buffer, buf + mov_puntero, sizeof(uint32_t));
					valor=actualizarCPUlibre(socket);
					if(valor==0){
					bloquearPCBaSemaforo(socket,tamanio_buffer);
					}
					else{
					eliminarDatosNoRelevantesSem(socket,tamanio_buffer);
					finalizarPCB(socket);
					//aca debería obtener el ID del proceso y hacer send a la UMC para que libere la memoria asociada al mismo
					}
				break;
				case IMPRIMIR_TEXTO:
					memcpy(&tamanio_buffer, buf + mov_puntero, sizeof(uint32_t));
					mandarAImprimirTexto(socket,tamanio_buffer);
				break;
				case FIN_QUANTUM:
					memcpy(&tamanio_buffer, buf + mov_puntero, sizeof(uint32_t));
					valor=actualizarCPUlibre(socket);
					if(valor==0){
					mandarAready(socket,tamanio_buffer);//tengo que deserializar el pcb y mandarlo a ready
					}
					else{
					finalizarPCBTradicional(socket,tamanio_buffer);
					//en el metodo finalizarPCB debería obtener el ID del proceso y hacer send a la UMC para que libere la memoria asociada al mismo
					}
				break;
				case FIN_CPU:
					memcpy(&tamanio_buffer, buf + mov_puntero, sizeof(uint32_t));
					eliminarCPU(socket);
				break;
			}
		break;
	}
	free (buf);
}

void handShake_Nucleo(uint32_t cliente, int* n) {
switch ((int) cliente) {
case ID_CONSOLA:
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
int nBytes = recv(socket, &idCliente, sizeof(uint32_t), 0);
if (nBytes <= 0) {
	return 0;
}
int n;
switch ((int) idReceptor) {
case ID_NUCLEO:
	handShake_Nucleo(idCliente, &n);
	return n;
	break;
default:
	printf("hubo un error\n");
	return 0;
	break;
}
}

void buscarCPUyAsignar(t_PCB* PCB){
	int i=0;
	int acierto=0;
	while((i<list_size(listaCPUs))&&acierto!=1){
		t_CPU* cpu=list_get(listaCPUs,i);
		if(cpu->estado==1){
			acierto=1;
			cpu->estado=0;
			cpu->pcbAsoc=PCB->PID;
			serializarPCByEnviar(PCB,cpu->s_cpu);
		}
		i++;
	}
}

void asignarPCB(){
	pthread_mutex_lock(&mutexReady);
	t_PCB* PCB=queue_pop(readyPCBs);
	printf("obtuve el pcb: %d\n",PCB->PID);
	pthread_mutex_unlock(&mutexReady);
	//signal mutex cola de Ready
	//wait mutex lista de CPUs
	pthread_mutex_lock(&mutexCPUs);
	buscarCPUyAsignar(PCB);
	pthread_mutex_unlock(&mutexCPUs);
	//signal mutex lista de CPUs
}

void manejadorIO(void* arg){
	t_IO* ioValues= (t_IO*)arg;
	t_PCB* pcb=malloc(sizeof(t_PCB));
	while(1){
		sem_wait(&ioValues->semIO);
		pthread_mutex_lock(&ioValues->mutexIO);
		pcb=queue_pop(ioValues->colIO);
		pthread_mutex_unlock(&ioValues->mutexIO);
		usleep(ioValues->retardo*pcb->cantOperacIO);
		pcb->cantOperacIO=0;
		pthread_mutex_lock(&mutexReady);
		queue_push(readyPCBs,pcb);
		pthread_mutex_unlock(&mutexReady);
	}
}

void PlanifCortoPlazo(){
	while(1){
		//aca hago wait de pcbs en ready
		sem_wait(&semReady);
		//aca hago otro wait de cpu libre
		sem_wait(&semCpu);
		printf("Arranca el Round Robin\n");
		asignarPCB();
	}
}

void handshakeUMC(){
	uint32_t id=ID_NUCLEO;
	send(UMC,&id,sizeof(uint32_t),0);
	recv(UMC,&tamPag,sizeof(uint32_t),0);
	//printf("%d\n",tamPag);
}

int main(int argc, char* argv[]) {
if (argc != 3) {
	printf("Cantidad Invalida de argumentos\n");
	return 1;
}
cfgNUCLEO = malloc(sizeof(t_config));
cfgNUCLEO = config_create(argv[1]);
inicializarEstructuras();
pthread_attr_t attr;
pthread_t PCP;
pthread_t* hilosIO= malloc(sizeof(pthread_t)*list_size(lIO));

//incialializo semaforos contadores cpu nueva y pcb nuevo en ready

pthread_mutex_init(&mutexCPUs,NULL);
pthread_mutex_init(&mutexQsleep,NULL);
pthread_mutex_init(&mutexReady,NULL);
sem_init(&semReady,0,0);
sem_init(&semCpu,0,0);


//Arranca la creación del select
iNotify= inotify_init();
int watch_descriptor = inotify_add_watch(iNotify, argv[2], IN_CLOSE_WRITE);
struct sockaddr_in direccionUMC;
uint32_t id;
crearCliente(&direccionUMC,config_get_int_value(cfgNUCLEO,"PUERTO_UMC"),config_get_string_value(cfgNUCLEO,"IP_UMC"));
UMC= socket(AF_INET, SOCK_STREAM, 0);
if (connect(UMC, (void*) &direccionUMC, sizeof(direccionUMC))
			!= 0) {
		perror("No se pudo conectar a la UMC\n");
		return 1;
}
handshakeUMC();
fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
struct sockaddr_in direccionServidor; // dirección del servidor
struct sockaddr_in remoteaddr; // dirección del cliente
int fdmax;        // número máximo de descriptores de fichero
int listener;     // descriptor de socket a la escucha
int newfd;        // descriptor de socket de nueva conexión aceptada
int valHand;
int nbytes;
int yes = 1;        // para setsockopt() SO_REUSEADDR, más abajo
int addrlen;
int i;

FD_ZERO(&master);    // borra los conjuntos maestro y temporal
FD_ZERO(&read_fds);
// obtener socket a la escucha
if ((listener = crearServer(&direccionServidor,
		config_get_int_value(cfgNUCLEO, "PUERTO_CONEXION"))) == -1) {
	perror("socket");
	exit(1);
}
// obviar el mensaje "address already in use" (la dirección ya se está usando)
if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
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
FD_SET(iNotify,&master);
// seguir la pista del descriptor de fichero mayor
fdmax = listener; // por ahora es éste
//Si llegue hasta aca, me pude conectar con la UMC y levantar le socket de escucha
//Entonces creo los hilos pertinentes
pthread_attr_init(&attr);
pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
pthread_create(&PCP,&attr,(void*)PlanifCortoPlazo,NULL);
int j=0;
while(j<list_size(lIO)){
	pthread_create(&hilosIO[j],&attr,(void*)manejadorIO,(void*)list_get(lIO,j));
	j++;
}

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
				if ((newfd = accept(listener, (struct sockaddr *) &remoteaddr,
						&addrlen)) == -1) {
					perror("accept");
				} else {
					valHand = handShake(newfd, ID_NUCLEO);
					if (valHand == 2) {
						printf("recibi una Consola en: %d\n", newfd);
						FD_SET(newfd, &master); // añadir al conjunto maestro
						if (newfd > fdmax) {    // actualizar el máximo
							fdmax = newfd;
						}
					} else if (valHand == 1) {
						printf("recibi una Cpu en: %d\n", newfd);
						FD_SET(newfd, &master); // añadir al conjunto maestro
						if (newfd > fdmax) {    // actualizar el máximo
							fdmax = newfd;
						}
						t_CPU* newCPU=malloc(sizeof(t_CPU));
						newCPU->s_cpu=newfd;
						newCPU->estado=1;
						newCPU->q_asoc=quantum;
						newCPU->estadoPCB=0;
						//wait del mutex de la lista de CPUs
						pthread_mutex_lock(&mutexCPUs);
						list_add(listaCPUs,newCPU);
						pthread_mutex_unlock(&mutexCPUs);
						sem_post(&semCpu);
						//signal del mutex de la lista de CPUs
						//signal del semaforo de CPUs libres
					}
					else {
						printf("no sos una conexion valida\n");
						close(newfd);
					}
				}
			}else if(i==iNotify){
				char buffer[sizeof (struct inotify_event) + 100];
				read(i, buffer, sizeof (struct inotify_event) + 100);
				struct inotify_event *event = (struct inotify_event *) &buffer[0];
				t_config* cfgAux;
				if (event->mask & IN_CLOSE_WRITE){
					cfgAux=config_create(event->name);
					if(quantum!=config_get_int_value(cfgAux,"QUANTUM")){
					quantum=config_get_int_value(cfgAux,"QUANTUM");
					printf("El Quantum se actualizo a: %d\n",(int)quantum);
					}
					pthread_mutex_lock(&mutexQsleep);
					if(qSleep!=config_get_int_value(cfgAux,"QUANTUM_SLEEP")){
					qSleep=config_get_int_value(cfgAux,"QUANTUM_SLEEP");
					printf("El Quantum Sleep se actualizo a: %d\n",(int)qSleep);
					}
					pthread_mutex_unlock(&mutexQsleep);
					config_destroy(cfgAux);
				}
			}
			else {
				buf=malloc(sizeof(uint32_t)*3);
				// gestionar datos de un cliente
				if ((nbytes = recv(i, buf, sizeof(uint32_t)*3, 0)) <= 0) {
					if (nbytes == 0) {
						// conexión cerrada
						printf("selectserver: socket %d hung up\n", i);
					} else {
						perror("recv\n");
					}
					close(i); // bye!
					FD_CLR(i, &master); // eliminar del conjunto maestro
					free(buf);
				} else {
					int tamanio=0;
					memcpy(&id,buf,sizeof(uint32_t));
					tamanio+=sizeof(uint32_t);
					atenderOrdenSegunID(i,id,tamanio);
				}
			}
		}
	}
}
pthread_attr_destroy(&attr);
return 0;
}
