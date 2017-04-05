/*
 * estructurasUMC.h
 *
 *  Created on: 27/5/2016
 *      Author: utnso
 */

#ifndef ESTRUCTURASUMC_H_
#define ESTRUCTURASUMC_H_

#include <commons/collections/list.h>
#include <stdint.h>
struct nodo_lista_frames {
uint32_t id_frame;
uint32_t pid;
//uint32_t nro_pagina;
uint8_t bitReferencia;
uint8_t bitModificado;
//uint8_t paginaModificada;
};

typedef struct nodo_lista_frames t_nodo_lista_frames;

struct nodo_lista_paginas {
uint32_t id_frame;
uint32_t nro_pagina;//?
char status; // M = memoria, S = SWAP
};

typedef struct nodo_lista_paginas t_nodo_lista_paginas;

struct nodo_lista_procesos {
uint32_t pid;
uint8_t cantPaginas;
uint8_t cantFramesAsignados;
//uint8_t paginasAsignadas;
int indiceRecorridaClock;
t_list * lista_paginas;
};

typedef struct nodo_lista_procesos t_nodo_lista_procesos;


typedef struct{
	uint32_t nro_pagina;
	uint32_t id_frame;
	uint32_t pid;
	uint16_t ultimo_acceso;
}t_entrada_TLB;

#endif /* ESTRUCTURASUMC_H_ */
