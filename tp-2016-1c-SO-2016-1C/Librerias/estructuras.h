/*
 * estructuras.h
 *
 *  Created on: 19/5/2016
 *      Author: utnso
 */

#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#include <parser/metadata_program.h>
#include <commons/collections/list.h>
#include <stdint.h>
typedef struct {
	uint32_t PID;
	uint32_t orden;  //0=INICIAR, 1=LEER, 2=ESCRIBIR, 3=FINALIZAR
	uint32_t cantPaginas;
	uint32_t nroPagina;
	char* contenido;
} OrdenProceso;

//Inicio Estructuras del PCB
typedef struct {
	t_size etiquetas_size; // Tamaño del mapa serializado de etiquetas
	char* etiquetas;  // La serializacion de las etiquetas
} t_indice_etiquetas;

typedef struct {
	uint32_t pag;
	uint32_t offset;
} t_posMemoria;

typedef struct {
	char ID;
	t_posMemoria posicionEnMemoria;
} t_variable;

typedef struct {
	uint32_t cantArgs;
	t_list* args;  //argumentos que recibe por parametro la funcion
	uint32_t cantVars;
	t_list* vars;  //variables de la funcion
	uint32_t retPos; // posicion a la cual retornar para seguir con el curso del programa
	t_posMemoria retVar;  //donde guardar si retorna un valor
} t_stack;

typedef struct {
	uint32_t PID;
	uint32_t PC;
	uint32_t paginasCodigo;
	uint32_t paginasStack;
	uint32_t cantOperacIO;

	t_size tamanioIndiceDeCodigo;
	t_intructions* indiceDeCodigo;

	t_indice_etiquetas indiceDeEtiquetas;

	uint32_t SP;
	t_size tamanioIndiceStack;
	t_list* indiceDeStack;
} t_PCB;
//Fin Estructuras PCB

//formato para la Lista de CPUs asociadas al Nucleo
typedef struct {
	u_int32_t s_cpu;
	u_int32_t q_asoc; //debería inicializarse con el quantum del archivo de config y cada vez que me mande la orden PASO_QUANTUM,
	//le resto uno a esta variable hasta llegar a 0 donde le pido que me de el PCB
	//y lo vuelvo a poner como libre a espera de otro PCB a ejecutar
	u_int32_t pcbAsoc; 		//si esta en 0 significa que no tiene un pcbAsociado
	u_int8_t estadoPCB; //este valor lo agrego para que si se me pide finalizar un proceso que se esta ejecutando, marcar con este bit eso
						//y cuando la cpu me devuelva el pcb revisar este bit para ver si lo tengo que finalizar o lo mando a la cola correspondiente
	u_int32_t estado; //0 ocupado, 1 libre
} t_CPU;

typedef enum {
	ID_NUCLEO,
	ID_CPU,
	ID_CONSOLA,
	ID_UMC,
	ID_SWAP,
	EXITO_SWAP,
	ERROR_SWAP,
	INICIALIZAR_PROGRAMA_UMC, //El nucleo indica a umc que inicia un programa.
	FINALIZAR_PROGRAMA_UMC, //El nucleo indica a umc que termino el programa, tambien puede usarlo la UMC para avisar.
							//al nucleo que termine un programa(Se puede usar cuando no hay frames para realizar el algoritmo de clock, que conlleva a matar el proceso)
	ERROR_OVERFLOW,	//Indica al nucleo y cpu que hubo overflow y que ellos se dediquen a matar o cerrar ese proceso
	PROGRAMA_INICIALIZADO,//Mensaje que le envia UMC al Nucleo para indicarle que el programa se inicializo correctamente
	PROGRAMA_NO_INICIALIZADO,//Mensaje que le envia UMC al Nucleo para indicarle que el programa no se pudo inicializar
	SOLICITAR_BYTES_UMC,//Mensaje que le enviaria la cpu a la umc para pedirle bytes
	ALMACENAR_BYTES_UMC,//Mensaje que le enviaria la cpu a la umc para almacenar bytes
	CAMBIO_DE_PROCESO,//Mensaje que le enviaria la cpu a la umc para indicar un cambio de proceso(Requiere enviar el pid nuevo)
	OK_SOLICITUD_UMC//Mensaje para indicar que la solicitud fue satisfactoria.
} identificadores;

typedef enum {
	INICIAR, LEER, ESCRIBIR, FINALIZAR
} orden;

typedef enum {
	INICIALIZAR_ANSISOP,
	FINALIZAR_ANSISOP,
	PASO_QUANTUM,
	FIN_ANSISOP,
	OBTENER_VALOR,
	GRABAR_VALOR,
	WAIT,
	SIGNAL,
	ENTRADA_SALIDA,
	BLOQUEAR_PCB_SEMAFORO,
	IMPRIMIR,
	IMPRIMIR_TEXTO,
	FIN_QUANTUM,
	FIN_CPU
} ordenesNUCLEO;

typedef enum {
	BLOQUEADO,
	NO_BLOQUEADO,
	ENVIO_PCB,
	DEVOLVER_PCB,
	SEGUIR_EJECUTANDO,
	VALOR_GLOBAL,
} ordenesCPU;

typedef enum {
	PROGRAMA_FINALIZO
} ordenesConsola;

#endif /* ESTRUCTURAS_H_ */
