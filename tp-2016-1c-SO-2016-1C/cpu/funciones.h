#include <stdlib.h>
#include <stdio.h>
#ifndef FUNCIONES_H_
#define FUNCIONES_H_

#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <commons/config.h>
#include <commons/string.h>
#include <sockets.h>
#include <estructuras.h>
#include <parser/parser.h>
#include <parser/metadata_program.h>
#include "globales.h"

void finAnsisop();
void serializarImprimir(void*buffer,int PID,char* textoImprimir,uint32_t longitud);
void enviarImprimir(char*textoImprimir,int pid,int longitud);
void serializarObtenerValor(void*buffer,char*variable,uint32_t longitud);
t_valor_variable solicitarValorVarCompartida(int nucleo,t_nombre_compartida variable);
void serializarAsignarValor(void*buffer,t_nombre_compartida variable, t_valor_variable valor);
void enviarAsignarValor(t_nombre_compartida variable, t_valor_variable valor);
void enviarPCBparaNucleo(int socketDestino,uint32_t accion,void*buffer);
void serializarWait(void*buffer,char* identificador_sem);
void avisoWait(int nucleo,t_nombre_semaforo identificador_semaforo);
void serializarSignal(void*buffer,char* identificador_sem);
void avisoSignal(int nucleo,t_nombre_semaforo identificador_semaforo);
void serializarEntradaSalida(void*buffer,char* dispositivo,int tiempo);
void avisoEntradaSalida(int nucleo,char* dispositivo,int tiempo);
void agregarPBCYEnviar(int socketDestino,void*buffer);
void serializarYEnvioPCB(int socketDestino,uint32_t accion,void*buffer);
t_puntero buscarVariable(t_stack* stackAux,t_nombre_variable identificador_variable);
void* solicitarBytesUMC( int socket,uint32_t numPag,uint32_t offset,uint32_t tamanio);
void serializarPedidoUMC(void*buffer,uint32_t numPag,uint32_t offset,uint32_t tamanio);
void almacenarBytesUMC(int socket,uint32_t numPag,uint32_t offset,uint32_t tamanio, t_valor_variable valor);
void recibirPCBparaCPU(int socketOrigen) ;

#endif /* FUNCIONES_H_ */
