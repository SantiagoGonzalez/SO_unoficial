
#ifndef GLOBALES_H_
#define GLOBALES_H_


#include <estructuras.h>
#include <commons/config.h>


int fin_cpu;
int sigo_ejecutando;
t_config* cfgCPU;
t_metadata_program* metaProg;
int nucleo;
char*bufferPCB;
t_PCB* pcb;
int umc;
uint32_t tamanioPagina;
uint32_t id_cpu;
uint32_t* quantum_sleep;
uint32_t tamanioHastaAhora;
int flagSaltoLinea;

#endif /* GLOBALES_H_ */
