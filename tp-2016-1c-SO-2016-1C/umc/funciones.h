/*
 * funciones.h
 *
 *  Created on: 27/5/2016
 *      Author: utnso
 */

#ifndef FUNCIONES_H_
#define FUNCIONES_H_

#include<stdint.h>
#include<stdlib.h>
#include"estructurasUMC.h"
#include"variablesGlobalesUMC.h"
#include<commons/collections/list.h>
#include<commons/txt.h>
#include<estructuras.h>
#include<commons/config.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "hexdump.h"
void inicializarPrograma(uint32_t pid,int cantPaginas,char* codPrograma);
void solicitarBytes(int numPagina,int offset,int tamaniooPag,uint32_t pid,int clienteCPU);
void almacenarBytesPagina(int numPagina,int offset,int tamaniooPag,void*buffer,uint32_t pid,int clienteCPU);
void finalizarPrograma(uint32_t pid);
void inicializarListasYVariables(t_config* cfgUMC);
#endif /* FUNCIONES_H_ */
