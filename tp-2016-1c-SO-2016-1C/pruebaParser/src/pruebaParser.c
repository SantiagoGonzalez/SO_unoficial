/*
 ============================================================================
 Name        : pruebaParser.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <commons/string.h>
#include <parser/parser.h>
#include <parser/metadata_program.h>

typedef struct {
  t_size etiquetas_size; // Tamaño del mapa serializado de etiquetas
  char* etiquetas;  // La serializacion de las etiquetas
 } t_indice_etiquetas;

 typedef struct {
  uint32_t pag;
  uint32_t offset;
  uint32_t size;
 } t_posMemoria;

 typedef struct {
  char ID;
  t_posMemoria posicionEnMemoria;
 } t_variable;

 typedef struct {
  t_posMemoria* args;
  t_variable* vars;
  uint32_t retPos;
  t_posMemoria retVar;
 } t_stack;

typedef struct {
  uint32_t PID;
  uint32_t PC;
  uint32_t cantidadDePaginas;

  t_size tamanioIndiceDeCodigo;
  t_intructions* indiceDeCodigo;

  t_indice_etiquetas indiceDeEtiquetas;

  uint32_t SP;
  t_size tamanioIndiceDeStack;
  t_stack* indiceDeStack;
 } t_PCB;

void finalizar(void){
	printf("hola soy finalizar");
}

void definirVariable(t_nombre_variable identificador_variable){
	printf("hola soy definirVariable\n");
}

void obtenerPosicionVariable(t_nombre_variable identificador_variable){
	printf("hola soy obtenerPosicionVariable\n");
}

void dereferenciar (t_puntero direccion_variable){
	printf("hola soy derefenciar\n");
}

void asignar(t_puntero direccion_variable, t_valor_variable valor){
	printf("hola soy asignar\n");
}

void obtenerValorCompartida(t_nombre_compartida variable){
	printf("hola soy obtenerValorCompartida\n");
}

void asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	printf("hola soy asignarValorCompartida\n");
}

void irAlLabel(t_nombre_etiqueta t_nombre_etiqueta){
	printf("hola soy irAlLabel\n");
}

void llamarSinRetorno(t_nombre_etiqueta etiqueta){
	printf("hola soy llamarSinRetorno\n");
}

void llamarConRetorno(t_nombre_etiqueta etiqueta){
	printf("hola soy llamarConRetorno\n");
}

void retornar(t_valor_variable retorno) {
	printf("hola soy retornar\n");
}

void imprimir(t_valor_variable valor_mostrar) {
	printf("hola soy imprimir\n");
}

void imprimirTexto(char*texto) {
	printf("hola soy imprimirTexto\n");
}

void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo) {
	printf("hola soy entradaSalida\n");
}

void wait(t_nombre_semaforo identificador_semaforo) {
	printf("hola soy wait\n");
}

void signal(t_nombre_semaforo identificador_semaforo) {
	printf("hola soy signal\n");
}
AnSISOP_funciones functions = {
		.AnSISOP_definirVariable		= definirVariable,
		.AnSISOP_obtenerPosicionVariable = obtenerPosicionVariable,
		.AnSISOP_dereferenciar			= dereferenciar,
		.AnSISOP_asignar				= asignar,
		.AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
		.AnSISOP_asignarValorCompartida = asignarValorCompartida,
		.AnSISOP_irAlLabel				= irAlLabel,
		.AnSISOP_llamarSinRetorno		= llamarSinRetorno,
		.AnSISOP_llamarConRetorno		= llamarConRetorno,
		.AnSISOP_retornar				= retornar,
		.AnSISOP_imprimir				= imprimir,
		.AnSISOP_imprimirTexto			= imprimirTexto,
		.AnSISOP_entradaSalida			= entradaSalida,
		.AnSISOP_finalizar				= finalizar,
};

AnSISOP_kernel kernel_functions = {
		.AnSISOP_wait					= wait,
		.AnSISOP_signal					= signal,
};

void agregarCentinela(char* linea){
	int i=strlen(linea);
	linea[i]='\0';
	printf("%s",linea);
}

int obtenerTamanioArchivo(FILE* arch){
	fseek(arch, 0, SEEK_END);
	int size = ftell(arch);
	fseek(arch, 0, SEEK_SET);
	return size;
}
char* obtenerLinea(FILE* archivo){
	fseek(archivo, 0, SEEK_END);
	int size = ftell(archivo);
	fseek(archivo, 0, SEEK_SET);
	char* linea=malloc(size+10);
	fread(linea,size,1,archivo);
	return linea;
}

int main(int argc, char* argv[]) {
	FILE*progAnsisop;
	t_metadata_program* metaProg;
	t_PCB* pcb=malloc(sizeof(t_PCB));
	if(argc!=2){
		printf("numero incorrecto de argumentos");
		return -1;
	}
	if((progAnsisop=fopen(argv[1],"r"))==NULL){
		printf("no se pudo abrir el archivo fuente");
		return -2;
	}
		char* linea=malloc(100);
		metaProg=metadata_desde_literal(obtenerLinea(progAnsisop));
		int i=0;
		pcb->PC=metaProg->instruccion_inicio;
		pcb->indiceDeCodigo=metaProg->instrucciones_serializado;
		free(metaProg);
		printf("%d\t %d\n",(int)pcb->PC,(int)pcb->indiceDeCodigo[0].start);
		while(i<metaProg->instrucciones_size){
			t_puntero_instruccion inicio=metaProg->instrucciones_serializado[pcb->PC].start;
			t_size offset=metaProg->instrucciones_serializado[pcb->PC].offset;
			fseek(progAnsisop,inicio,SEEK_SET);
			linea=fgets(linea,offset,progAnsisop);
			analizadorLinea(linea,&functions,&kernel_functions);
			pcb->PC+=1;
			i++;
		}
	return 0;
}
