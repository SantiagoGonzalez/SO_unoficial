/*
 * variablesGlobalesUMC.h
 *
 *  Created on: 27/5/2016
 *      Author: utnso
 */

#ifndef VARIABLESGLOBALESUMC_H_
#define VARIABLESGLOBALESUMC_H_

#include <commons/collections/list.h>
#include <commons/log.h>
#include<pthread.h>
t_list* listaDeProcesos;
t_list* listaDeFrames;
t_list* tlb;
pthread_mutex_t mutexTLB;
pthread_mutex_t mutexMarcos;
pthread_mutex_t mutexProcs;
pthread_mutex_t mutexSwap;
pthread_mutex_t mutexAccesoMem;
pthread_mutex_t mutexMP;
pthread_mutex_t mutexRetardo;
pthread_mutex_t mutexNucleo;
uint32_t accesosAMemoria;
uint16_t entradasTLB;
t_log* logger;
int socketSwap;
uint32_t marcos_x_proc;
uint32_t tamanioFrame;
uint32_t cantidadDeFrames;
int retardoUMC;
int tamanioTotalmemoria;
void* memoriaprincipal;
int socketNucleo;
char* algoritmo;
#endif /* VARIABLESGLOBALESUMC_H_ */
