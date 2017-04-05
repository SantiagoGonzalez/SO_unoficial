#include "funciones.h"

//ID_CPU,IMPRIMIR_TEXTO,long del char*,char*,PID
void serializarImprimir(void*buffer,int PID,char* textoImprimir,uint32_t longitud){

	memcpy(buffer,&id_cpu,sizeof(uint32_t));
	uint32_t imp_txt=(uint32_t) IMPRIMIR_TEXTO;
	memcpy(buffer+sizeof(uint32_t),&imp_txt,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t),&longitud,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t),textoImprimir,longitud);
	uint32_t pid=(uint32_t)PID;
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t)+longitud,&pid,sizeof(uint32_t));

}
void enviarImprimir(char*textoImprimir,int pid,int longitud){
	uint32_t auxlongitud=(uint32_t)longitud;

	int cantidadEnviar=sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t)+auxlongitud;
	void*buffer=malloc(cantidadEnviar);

	serializarImprimir(buffer,pid,textoImprimir,auxlongitud);
	//Envio tamanio + pcb+texto

	send(nucleo,buffer,cantidadEnviar,0);
	free(buffer);
}

//ID_CPU,OBTENER_VALOR,tamanio nombre de la var,char* variable
void serializarObtenerValor(void*buffer,char*variable,uint32_t longitud){
	memcpy(buffer,&id_cpu,sizeof(uint32_t));
	uint32_t obt_val=(uint32_t) OBTENER_VALOR;
	memcpy(buffer+sizeof(uint32_t),&obt_val,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t),&longitud,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t),variable,longitud);
}
t_valor_variable solicitarValorVarCompartida(int nucleo,t_nombre_compartida variable){
	uint32_t tamanio=(uint32_t)(strlen(variable)+1);
	int tamanioAEnviar=sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t)+tamanio;
	void*buffer=malloc(tamanioAEnviar);
	serializarObtenerValor(buffer,variable,tamanio);
	send(nucleo,buffer,tamanioAEnviar,0);
	t_valor_variable valor;
	recv(nucleo,&valor,sizeof(t_valor_variable),0);
	free(buffer);
	return valor;
}

//ID_CPU,GRABAR_VALOR,tamanio del valor de la variable mas el nombre de la variable, valor de la variable, nombre de la variable
void serializarAsignarValor(void*buffer,t_nombre_compartida variable, t_valor_variable valor){
	memcpy(buffer,&id_cpu,sizeof(uint32_t));
	uint32_t grab_val=(uint32_t) GRABAR_VALOR;
	memcpy(buffer+sizeof(uint32_t),&grab_val,sizeof(uint32_t));
	uint32_t longitudVar= (uint32_t)(strlen(variable)+1);
	uint32_t longitudTotal= longitudVar+sizeof(valor);
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t),&longitudTotal,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t),&valor,sizeof(valor));
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(valor),variable,longitudVar);

}
void enviarAsignarValor(t_nombre_compartida variable, t_valor_variable valor){
	uint32_t longitudVar= (uint32_t)(strlen(variable)+1);
	int tamanioAEnviar=sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t)+longitudVar;
	void*buffer=malloc(tamanioAEnviar);
	serializarAsignarValor(buffer,variable,valor);
	send(nucleo,buffer,tamanioAEnviar,0);
	free(buffer);
}

//ID_CPU,WAIT,tamanio nombre del semaforo,char*nombre semaforo
void serializarWait(void*buffer,char* identificador_sem){
	memcpy(buffer,&id_cpu,sizeof(uint32_t));
	tamanioHastaAhora+=sizeof(uint32_t);
	uint32_t wait=(uint32_t) WAIT;
	memcpy(buffer+tamanioHastaAhora,&wait,sizeof(uint32_t));
	tamanioHastaAhora+=sizeof(uint32_t);
	uint32_t tamanio= (uint32_t)(strlen(identificador_sem)+1);
	memcpy(buffer+tamanioHastaAhora,&tamanio,sizeof(uint32_t));
	tamanioHastaAhora+=sizeof(uint32_t);
	memcpy(buffer+tamanioHastaAhora,identificador_sem,tamanio);
	tamanioHastaAhora+=tamanio;
}
void avisoWait(int nucleo,t_nombre_semaforo identificador_semaforo){
	tamanioHastaAhora=0;
	void*buffer=malloc(60000);
	serializarWait(buffer,identificador_semaforo);
	send(nucleo,buffer,tamanioHastaAhora,0);
	uint32_t resultado;
	recv(nucleo,&resultado,sizeof(uint32_t),0);
	//BLOQUEADO o NO_BLOQUEADO
	if (resultado==BLOQUEADO) {
		enviarPCBparaNucleo(nucleo,(uint32_t)BLOQUEAR_PCB_SEMAFORO,buffer);
		sigo_ejecutando=0;
	}
	else{
		free(buffer);
	}

}

//ID_CPU,SIGNAL,tamanio nombre del semaforo,char*nombre semaforo
void serializarSignal(void*buffer,char* identificador_sem){

	memcpy(buffer,&id_cpu,sizeof(uint32_t));
	uint32_t wait=(uint32_t) SIGNAL;
	memcpy(buffer+sizeof(uint32_t),&wait,sizeof(uint32_t));
	uint32_t tamanio= (uint32_t)(strlen(identificador_sem)+1);
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t),&tamanio,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t),identificador_sem,tamanio);
}
void avisoSignal(int nucleo,t_nombre_semaforo identificador_semaforo){
	uint32_t tamanio= (uint32_t)(strlen(identificador_semaforo)+1);
	int tamanioAEnviar=sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t)+tamanio;
	void*buffer=malloc(tamanioAEnviar);
	serializarWait(buffer,identificador_semaforo);
	send(nucleo,buffer,tamanioAEnviar,0);
	free(buffer);

}

//ID_CPU,ENTRADA_SALIDA,tamaño dispositivo, dispositivo--tam pcb, pcb
void serializarEntradaSalida(void*buffer,char* dispositivo,int tiempo){
	memcpy(buffer,&id_cpu,sizeof(uint32_t));
	tamanioHastaAhora+=sizeof(uint32_t);
	uint32_t es=(uint32_t) ENTRADA_SALIDA;
	memcpy(buffer+tamanioHastaAhora,&es,sizeof(uint32_t));
	tamanioHastaAhora+=sizeof(uint32_t);
	uint32_t tamanioDisp= (uint32_t)(strlen(dispositivo)+1);
	memcpy(buffer+tamanioHastaAhora,&tamanioDisp,sizeof(uint32_t));
	tamanioHastaAhora+=sizeof(uint32_t);
	memcpy(buffer+tamanioHastaAhora,dispositivo,tamanioDisp);
	tamanioHastaAhora+=tamanioDisp;

}
void avisoEntradaSalida(int nucleo,char* dispositivo,int tiempo){
	tamanioHastaAhora=0;
	void*buffer=malloc(6000);
	serializarEntradaSalida(buffer,dispositivo,tiempo);
	enviarPCBparaNucleo(nucleo,(uint32_t)ENTRADA_SALIDA,buffer);
}

//ObtenerPosicionVariable
t_puntero buscarVariable(t_stack* stackAux,t_nombre_variable identificador_variable){
	int i=0;
	int acierto=0;
	t_puntero posicion;
	t_variable* varAux;
	if((identificador_variable>='0')&&(identificador_variable<='9')){
		while((i<list_size(stackAux->args))&&(acierto==0)){
			varAux=list_get(stackAux->args,i);
			if (varAux->ID==identificador_variable) {
				acierto=1;
				posicion=((varAux->posicionEnMemoria.pag)*tamanioPagina)+(varAux->posicionEnMemoria.offset);
			}
			i++;
		}
	}
	else{
		while((i<list_size(stackAux->vars))&&(acierto==0)){
			varAux=list_get(stackAux->vars,i);
			if (varAux->ID==identificador_variable) {
				acierto=1;
				posicion=((varAux->posicionEnMemoria.pag)*tamanioPagina)+(varAux->posicionEnMemoria.offset);
			}
			i++;
		}

	}
	if (acierto==0) {
		return -1;
	}
	return posicion;
}

//ORDEN+pagina+offset+tamaño
void serializarPedidoUMC(void*buffer,uint32_t numPag,uint32_t offset,uint32_t tamanio){
	uint32_t orden=(uint32_t) SOLICITAR_BYTES_UMC;
	memcpy(buffer,&orden,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t),&numPag,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t),&offset,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t),&tamanio,sizeof(uint32_t));

}
void* solicitarBytesUMC( int socket,uint32_t numPag,uint32_t offset,uint32_t tamanio){
	void*linea=malloc(tamanio);
	int tamanioAEnviar=sizeof(uint32_t)*4;
	void* buffer=malloc(tamanioAEnviar);
	serializarPedidoUMC(buffer,numPag,offset,tamanio);
	send(umc,buffer,tamanioAEnviar,0);
	uint32_t orden;
	recv(umc,&orden,sizeof(uint32_t),0);
	switch (orden) {
		case OK_SOLICITUD_UMC:
			recv(umc,linea,tamanio,0);
			printf("Recibi bytes de UMC exitosamente\n");
			free(buffer);
			return linea;
			break;
		case ERROR_OVERFLOW:
			printf("No se pudo realizar la solicitud a la UMC\n");
			finAnsisop();
			sigo_ejecutando=0;
			free(buffer);
			return NULL;
			break;
		default:
			printf("No se pudo realizar la solicitud a la UMC\n");
			finAnsisop();
			sigo_ejecutando=0;
			free(buffer);
			abort();
			break;
	}
}

void almacenarBytesUMC(int socket,uint32_t numPag,uint32_t offset,uint32_t tamanio, t_valor_variable valor){
	void*buffer=malloc(sizeof(uint32_t)*5);
	uint32_t valorReal= (uint32_t) valor;
	uint32_t orden =(uint32_t) ALMACENAR_BYTES_UMC;
	memcpy(buffer,&orden,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t),&numPag,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t),&offset,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t),&tamanio,sizeof(uint32_t));
	memcpy(buffer+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t),&valorReal, sizeof(uint32_t));
	send(umc,buffer,sizeof(uint32_t)*5,0);
	uint32_t ordenRpta;
	recv(umc,&ordenRpta,sizeof(uint32_t),0);
	switch (ordenRpta) {
			case OK_SOLICITUD_UMC:
				printf("Variable Asignada\n");
				break;
			case ERROR_OVERFLOW:
				printf("No se pudo realizar la solicitud a la UMC\n");
				finAnsisop();
				sigo_ejecutando=0;
				break;
			default:
				printf("No se pudo realizar la solicitud a la UMC\n");
				finAnsisop();
				sigo_ejecutando=0;
				abort();
				break;
		}
	free(buffer);

}

//Recibo PCB
void recibirPCBparaCPU(int socketOrigen) {

	uint32_t accion;
	uint32_t tamPCB;
		printf("Espero el PCB de Nucleo\n");
		int bytesRecibidos = recv(socketOrigen, &accion, sizeof(uint32_t), 0);
		if (bytesRecibidos <= 0) {
			perror("El emisor se desconectó1.");
			abort();
		}
		if (accion == ENVIO_PCB) {

			//Recibo Quantum Sleep
			bytesRecibidos = recv(socketOrigen, &quantum_sleep, sizeof(uint32_t),
					0);
			if (bytesRecibidos <= 0) {
				perror("El emisor se desconectó2...");
			}

			//Recibo TamPCB(uint32)
			bytesRecibidos = recv(socketOrigen, &tamPCB, sizeof(uint32_t),
								0);
			if (bytesRecibidos <= 0) {
				perror("El emisor se desconectó3...");
			}

			pcb=malloc(tamPCB);

			//Recibo PID
			bytesRecibidos = recv(socketOrigen, &(pcb->PID), sizeof(uint32_t),
					0);
			if (bytesRecibidos <= 0) {
				perror("El emisor se desconectó4.");
			}

			//Recibo PC
			bytesRecibidos = recv(socketOrigen, &(pcb->PC), sizeof(uint32_t),
					0);
			if (bytesRecibidos <= 0) {
				perror("El emisor se desconectó5.");
			}

			//Recibo PaginasCodigo
			bytesRecibidos = recv(socketOrigen, &(pcb->paginasCodigo),sizeof(uint32_t), 0);
			if (bytesRecibidos <= 0) {
				perror("El emisor se desconectó6.");
			}

			//Recibo PaginasStack
			bytesRecibidos = recv(socketOrigen, &(pcb->paginasStack),sizeof(uint32_t), 0);
			if (bytesRecibidos <= 0) {
				perror("El emisor se desconectó6.");
			}

			//Recibo CantidadDeOperacionesEntradaSalida
			bytesRecibidos=recv(socketOrigen,&(pcb->cantOperacIO),sizeof(uint32_t),0);
			if(bytesRecibidos<=0){
						perror("El emisor se desconectó7.");
					}

			//Recibo Tamanio Indice de Codigo
			bytesRecibidos = recv(socketOrigen, &(pcb->tamanioIndiceDeCodigo),
					sizeof(uint32_t), 0);
			if (bytesRecibidos <= 0) {
				perror("El emisor se desconectó8.");
			}

			//Recibo Indice de Codigo
			pcb->indiceDeCodigo = malloc(pcb->tamanioIndiceDeCodigo*sizeof(t_intructions));
			int i=0;
			while(i<pcb->tamanioIndiceDeCodigo){
				recv(socketOrigen, &(pcb->indiceDeCodigo[i].start),sizeof(uint32_t), 0);
				recv(socketOrigen, &(pcb->indiceDeCodigo[i].offset),sizeof(uint32_t), 0);
				i++;
			}

			/*pcb->indiceDeCodigo = malloc(pcb->tamanioIndiceDeCodigo);

			bytesRecibidos = recv(socketOrigen, pcb->indiceDeCodigo,
					pcb->tamanioIndiceDeCodigo, 0);
			if (bytesRecibidos <= 0) {
				perror("El emisor se desconectó9.");
			}*/

			//Recibo Tamanio Indice de Etiquetas

			bytesRecibidos = recv(socketOrigen,
					&(pcb->indiceDeEtiquetas.etiquetas_size), sizeof(uint32_t),
					0);
			if (bytesRecibidos <= 0) {
				perror("El emisor se desconectó10.");
			}

			if (pcb->indiceDeEtiquetas.etiquetas_size > 0) {
				//Recibo Indice de Etiquetas
				pcb->indiceDeEtiquetas.etiquetas = malloc(
						pcb->indiceDeEtiquetas.etiquetas_size);

				bytesRecibidos = recv(socketOrigen,
						pcb->indiceDeEtiquetas.etiquetas,
						pcb->indiceDeEtiquetas.etiquetas_size, 0);
				if (bytesRecibidos <= 0) {
					perror("El emisor se desconectó11.");
				}
			}
			//Recibo Stack Pointer

			bytesRecibidos = recv(socketOrigen, &(pcb->SP), sizeof(uint32_t),
					0);
			if (bytesRecibidos <= 0) {
				perror("El emisor se desconectó12.");
			}

			//Recibo Tamanio Indice de Stack
			bytesRecibidos = recv(nucleo, &(pcb->tamanioIndiceStack),sizeof(uint32_t), 0);
			if (bytesRecibidos <= 0) {
					perror("El emisor se desconectó11.");
				}

			if (pcb->tamanioIndiceStack > 0) {

			//Recibo Indice de Stack
				pcb->indiceDeStack = list_create();
				int i=0;
				while(i<pcb->tamanioIndiceStack){
					t_stack* stackAux=malloc(sizeof(t_stack));
					//Recibo Argumentos
					recv(nucleo,&stackAux->cantArgs,sizeof(uint32_t),0);
					if(stackAux->cantArgs>0){
						recv(nucleo,stackAux->args,sizeof(t_list),0);
						stackAux->args=list_create();
						int j=0;
						while(j<(stackAux->cantArgs)){
							t_variable* argAux=malloc(sizeof(t_variable));
							recv(nucleo,&argAux->ID,sizeof(char),0);
							//recv(s_cpu,&argAux->posicionEnMemoria,sizeof(t_posMemoria),0);
							recv(nucleo,&argAux->posicionEnMemoria.pag,sizeof(uint32_t),0);
							recv(nucleo,&argAux->posicionEnMemoria.offset,sizeof(uint32_t),0);
							list_add(stackAux->args,argAux);
							j++;
						}
					}
					//Recibo Variables
					recv(nucleo,&stackAux->cantVars,sizeof(uint32_t),0);
					if(stackAux->cantVars>0){
						recv(nucleo,stackAux->vars,sizeof(t_list),0);
						stackAux->vars=list_create();
						int j=0;
						while(j<(stackAux->cantVars)){
							t_variable* varAux=malloc(sizeof(t_variable));
							recv(nucleo,&varAux->ID,sizeof(char),0);
							//recv(s_cpu,&varAux->posicionEnMemoria,sizeof(t_posMemoria),0);
							recv(nucleo,&varAux->posicionEnMemoria.pag,sizeof(uint32_t),0);
							recv(nucleo,&varAux->posicionEnMemoria.offset,sizeof(uint32_t),0);
							list_add(stackAux->args,varAux);
							j++;
						}
					}
					//Recibo Resto
					recv(nucleo,&stackAux->retPos,sizeof(uint32_t),0);
					recv(nucleo,&stackAux->retVar.pag,sizeof(uint32_t),0);
					recv(nucleo,&stackAux->retVar.offset,sizeof(uint32_t),0);
					list_add(pcb->indiceDeStack,stackAux);
					i++;
				}
			}
		printf("Recibi el PCB correctamente\n");
		}
		else{
			perror("Accion incorrecta");
		}
}

//FIN_Q Y FIN_ANSISOP
///ID_CPU,ORDEN,tam pbc,pcb
void serializarYEnvioPCB(int socketDestino,uint32_t accion,void*buffer){
	uint32_t tamanioValidoBuffer = 0;
	uint32_t headerOrigen = ID_CPU;

	//Copio Header
	memcpy(buffer, &headerOrigen, sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio Accion
	memcpy(buffer + tamanioValidoBuffer, &accion, sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio PID
	memcpy(buffer + tamanioValidoBuffer, &(pcb->PID), sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio PC
	memcpy(buffer + tamanioValidoBuffer, &(pcb->PC), sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio PaginasCodigo
	memcpy(buffer + tamanioValidoBuffer, &(pcb->paginasCodigo),sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio PaginasStack
	memcpy(buffer + tamanioValidoBuffer, &(pcb->paginasStack),sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio CantOperacIO
	memcpy(buffer+tamanioValidoBuffer,&(pcb->cantOperacIO),sizeof(uint32_t));
	tamanioValidoBuffer+=sizeof(uint32_t);

	//Copio Tamanio Indice de Codigo
	memcpy(buffer + tamanioValidoBuffer, &(pcb->tamanioIndiceDeCodigo), sizeof(uint32_t));
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
	/*memcpy(buffer + tamanioValidoBuffer, pcb->indiceDeCodigo,pcb->tamanioIndiceDeCodigo);
	tamanioValidoBuffer += pcb->tamanioIndiceDeCodigo;*/
	//Copio Etiquetas Size
	memcpy(buffer + tamanioValidoBuffer,&(pcb->indiceDeEtiquetas.etiquetas_size), sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio Etiquetas
	memcpy(buffer + tamanioValidoBuffer, pcb->indiceDeEtiquetas.etiquetas,pcb->indiceDeEtiquetas.etiquetas_size);
	tamanioValidoBuffer += pcb->indiceDeEtiquetas.etiquetas_size;

	//Copio SP
	memcpy(buffer + tamanioValidoBuffer, &(pcb->SP), sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio Tamanio Indice de Stack
	memcpy(buffer + tamanioValidoBuffer, &pcb->tamanioIndiceStack,sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	if(pcb->tamanioIndiceStack>0){
		int i=0;
		while(i<list_size(pcb->indiceDeStack)){
			t_stack* stack=malloc(sizeof(t_stack));
			stack=list_remove(pcb->indiceDeStack,i);
			memcpy(buffer + tamanioValidoBuffer, &stack->cantArgs,sizeof(uint32_t));
			tamanioValidoBuffer += sizeof(uint32_t);
			if(stack->cantArgs>0){
				int j=0;
				while(j<list_size(stack->args)){
					t_variable* arg=malloc(sizeof(t_variable));
					arg=list_remove(stack->args,j);
					memcpy(buffer + tamanioValidoBuffer, &arg->ID,sizeof(char));
					tamanioValidoBuffer += sizeof(char);
					memcpy(buffer + tamanioValidoBuffer, &arg->posicionEnMemoria.pag,sizeof(uint32_t));
					tamanioValidoBuffer += sizeof(uint32_t);
					memcpy(buffer + tamanioValidoBuffer, &arg->posicionEnMemoria.offset,sizeof(uint32_t));
					tamanioValidoBuffer += sizeof(uint32_t);
					j++;
				}
			}
			memcpy(buffer + tamanioValidoBuffer, &stack->cantVars,sizeof(uint32_t));
			tamanioValidoBuffer += sizeof(uint32_t);
			if(stack->cantVars>0){
				int j=0;
				while(j<list_size(stack->vars)){
					t_variable* var=malloc(sizeof(t_variable));
					var=list_remove(stack->vars,j);
					memcpy(buffer + tamanioValidoBuffer, &var->ID,sizeof(char));
					tamanioValidoBuffer += sizeof(char);
					memcpy(buffer + tamanioValidoBuffer, &var->posicionEnMemoria.pag,sizeof(uint32_t));
					tamanioValidoBuffer += sizeof(uint32_t);
					memcpy(buffer + tamanioValidoBuffer, &var->posicionEnMemoria.offset,sizeof(uint32_t));
					tamanioValidoBuffer += sizeof(uint32_t);
					j++;
				}
			}
		memcpy(buffer + tamanioValidoBuffer, &stack->retPos,sizeof(uint32_t));
		tamanioValidoBuffer += sizeof(uint32_t);
		memcpy(buffer + tamanioValidoBuffer, &stack->retVar.pag,sizeof(uint32_t));
		tamanioValidoBuffer += sizeof(uint32_t);
		memcpy(buffer + tamanioValidoBuffer, &stack->retVar.offset,sizeof(uint32_t));
		tamanioValidoBuffer += sizeof(uint32_t);
		i++;
		}
	}

	uint32_t tamanioPCB= tamanioValidoBuffer-(sizeof(uint32_t)*2);
	///Envio PCB a la CPU
	memcpy(buffer+(sizeof(uint32_t)*2),&tamanioPCB,sizeof(uint32_t));
	send(nucleo, buffer, tamanioValidoBuffer, 0);
	free(buffer);
}
//Agrego TamPCB+PCB
void agregarPBCYEnviar(int socketDestino,void*buffer){
	uint32_t tamanioValidoBuffer = tamanioHastaAhora;

	//Copio PID
	memcpy(buffer + tamanioValidoBuffer, &(pcb->PID), sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio PC
	memcpy(buffer + tamanioValidoBuffer, &(pcb->PC), sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio PaginasCodigo
	memcpy(buffer + tamanioValidoBuffer, &(pcb->paginasCodigo),sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio PaginasStack
	memcpy(buffer + tamanioValidoBuffer, &(pcb->paginasStack),sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio CantOperacIO
	memcpy(buffer+tamanioValidoBuffer,&(pcb->cantOperacIO),sizeof(uint32_t));
	tamanioValidoBuffer+=sizeof(uint32_t);

	//Copio Tamanio Indice de Codigo
	memcpy(buffer + tamanioValidoBuffer, &(pcb->tamanioIndiceDeCodigo),sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio Indice de Codigo
	memcpy(buffer + tamanioValidoBuffer, pcb->indiceDeCodigo,pcb->tamanioIndiceDeCodigo);
	tamanioValidoBuffer += pcb->tamanioIndiceDeCodigo;

	//Copio Etiquetas Size
	memcpy(buffer + tamanioValidoBuffer,&(pcb->indiceDeEtiquetas.etiquetas_size), sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio Etiquetas
	memcpy(buffer + tamanioValidoBuffer, pcb->indiceDeEtiquetas.etiquetas,pcb->indiceDeEtiquetas.etiquetas_size);
	tamanioValidoBuffer += pcb->indiceDeEtiquetas.etiquetas_size;

	//Copio SP
	memcpy(buffer + tamanioValidoBuffer, &(pcb->SP), sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	//Copio Tamanio Indice de Stack
	memcpy(buffer + tamanioValidoBuffer, &pcb->tamanioIndiceStack,sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);
	if(pcb->tamanioIndiceStack>0){
		int i=0;
		while(i<list_size(pcb->indiceDeStack)){
			t_stack* stack=malloc(sizeof(t_stack));
			stack=list_remove(pcb->indiceDeStack,i);
			memcpy(buffer + tamanioValidoBuffer, &stack->cantArgs,sizeof(uint32_t));
			tamanioValidoBuffer += sizeof(uint32_t);
			if(stack->cantArgs>0){
				int j=0;
				while(j<list_size(stack->args)){
					t_variable* arg=malloc(sizeof(t_variable));
					arg=list_remove(stack->args,j);
					memcpy(buffer + tamanioValidoBuffer, &arg->ID,sizeof(char));
					tamanioValidoBuffer += sizeof(char);
					memcpy(buffer + tamanioValidoBuffer, &arg->posicionEnMemoria.pag,sizeof(uint32_t));
					tamanioValidoBuffer += sizeof(uint32_t);
					memcpy(buffer + tamanioValidoBuffer, &arg->posicionEnMemoria.offset,sizeof(uint32_t));
					tamanioValidoBuffer += sizeof(uint32_t);
					j++;
				}
			}
			memcpy(buffer + tamanioValidoBuffer, &stack->cantVars,sizeof(uint32_t));
			tamanioValidoBuffer += sizeof(uint32_t);
			if(stack->cantVars>0){
				int j=0;
				while(j<list_size(stack->vars)){
					t_variable* var=malloc(sizeof(t_variable));
					var=list_remove(stack->vars,j);
					memcpy(buffer + tamanioValidoBuffer, &var->ID,sizeof(char));
					tamanioValidoBuffer += sizeof(char);
					memcpy(buffer + tamanioValidoBuffer, &var->posicionEnMemoria.pag,sizeof(uint32_t));
					tamanioValidoBuffer += sizeof(uint32_t);
					memcpy(buffer + tamanioValidoBuffer, &var->posicionEnMemoria.offset,sizeof(uint32_t));
					tamanioValidoBuffer += sizeof(uint32_t);
					j++;
				}
			}
			memcpy(buffer + tamanioValidoBuffer, &stack->retPos,sizeof(uint32_t));
			tamanioValidoBuffer += sizeof(uint32_t);
			memcpy(buffer + tamanioValidoBuffer, &stack->retVar.pag,sizeof(uint32_t));
			tamanioValidoBuffer += sizeof(uint32_t);
			memcpy(buffer + tamanioValidoBuffer, &stack->retVar.offset,sizeof(uint32_t));
			tamanioValidoBuffer += sizeof(uint32_t);
			i++;
		}
	}

	uint32_t tamanioPCB= tamanioValidoBuffer-(sizeof(uint32_t)*2);
	///Envio PCB a la CPU
	memcpy(buffer+(sizeof(uint32_t)*2),&tamanioPCB,sizeof(uint32_t));
	send(nucleo, buffer, tamanioValidoBuffer, 0);
	free(buffer);
}

//Envio PCB (Switch)
void enviarPCBparaNucleo(int socketDestino,uint32_t accion,void*buffer) {
	switch(accion){

	case ENTRADA_SALIDA:
		//ID_CPU,ENTRADA_SALIDA,tamaño dispositivo, dispositivo,tam pcb, pcb
		agregarPBCYEnviar(socketDestino,buffer);
		break;
	case BLOQUEAR_PCB_SEMAFORO:
		//ID_CPU,BLOQ_PCB_SEM,tam id_semaforo, nombre semaforo, tam pcb, pcb
		agregarPBCYEnviar(socketDestino,buffer);
		break;
	case FIN_QUANTUM:
		//ID_CPU,FIN_QUANTUM,tam pcb,pcb
		serializarYEnvioPCB(socketDestino,accion,buffer);
		break;
	case FIN_ANSISOP:
		//ID_CPU,FIN_ANSISOP,tam pbc,pcb
		serializarYEnvioPCB(socketDestino,accion,buffer);
		break;
	}
	printf("Le envie a Nucleo el PCB\n");
}

void finAnsisop(){
	void*buffer=malloc(60000);
	enviarPCBparaNucleo(nucleo,(uint32_t) FIN_ANSISOP,buffer);
}
