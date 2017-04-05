#include <parser/parser.h>
#include <parser/metadata_program.h>
#include <stdio.h>
#include <commons/log.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

//TODO: CAMBIAR LOS VALORES DE LOS RETURN
//TODO: CAMBIAR LOS VALORES DE LOS RETURN
//TODO: CAMBIAR LOS VALORES DE LOS RETURN

t_puntero definirVariable(t_nombre_variable identificador_variable) {
	printf("definirVariable %c\n", identificador_variable);
	return 0;

}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable) {
	printf("obtenerPosicionVariable %c\n", identificador_variable);
	return 0;

}

t_valor_variable dereferenciar(t_puntero direccion_variable) {
	printf("dereferenciar %d\n", direccion_variable);
	return 0;

}

void asignar(t_puntero direccion_variable, t_valor_variable valor) {
	printf("asignar a dir %d el valor %d\n", direccion_variable, valor);
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable) {
	printf("obtenerValorCompartida %c\n", variable);
	return 0;

}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable,
		t_valor_variable valor) {
	printf("asignarValorCompartida a var %c el valor %d\n ", variable, valor);
	return 0;

}

t_puntero_instruccion irAlLabel(t_nombre_etiqueta etiqueta) {
	printf("irAlLabel %c\n", etiqueta);
	return 0;

}

t_puntero_instruccion llamarFuncion(t_nombre_etiqueta etiqueta,
		t_puntero donde_retornar, t_puntero_instruccion linea_en_ejecuccion) {
	printf("llamarFuncion %c retornar a %d linea ejec %d\n", etiqueta,
			donde_retornar, linea_en_ejecuccion);
	return 0;

}

t_puntero_instruccion retornar(t_valor_variable retorno) {
	printf("retornar %d\n", retorno);
	return 0;

}

void imprimir(t_valor_variable valor_mostrar) {
	printf("imprimir %d\n", valor_mostrar);
}

void imprimirTexto(char*texto) {
	printf("imprimirTexto %c\n", texto);
}

int entradaSalida(t_nombre_dispositivo dispositivo, int tiempo) {
	printf("entradaSalida");
	return 0;
}

int wait(t_nombre_semaforo identificador_semaforo) {
	printf("wait %c\n", identificador_semaforo);
	return 0;
}

int signal(t_nombre_semaforo identificador_semaforo) {
	printf("signal %c\n", identificador_semaforo);
	return 0;
}

AnSISOP_funciones funciones = {
		.AnSISOP_definirVariable = definirVariable,
		.AnSISOP_obtenerPosicionVariable = obtenerPosicionVariable,
		.AnSISOP_dereferenciar = dereferenciar,
		.AnSISOP_asignar = asignar,
		.AnSISOP_imprimir = imprimir,
		.AnSISOP_imprimirTexto = imprimirTexto,
		.AnSISOP_irAlLabel = irAlLabel,
//		.AnSISOP_llamarFuncion = llamarFuncion,
		.AnSISOP_retornar = retornar,
		.AnSISOP_entradaSalida = entradaSalida,
		.AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
		.AnSISOP_asignarValorCompartida = asignarValorCompartida,


};

AnSISOP_kernel kernelFunc = {
		.AnSISOP_wait = wait,
		.AnSISOP_signal = signal,
 };

static const char* DEFINICION_VARIABLES = "variables a, b, c";

void correrDefinirVariables() {
	printf("Ejecutando '%s'\n", DEFINICION_VARIABLES);
	analizadorLinea(strdup(DEFINICION_VARIABLES), &funciones,&kernelFunc);
	printf("================\n");
}


int main(int argc, char **argv) {

	correrDefinirVariables();
	return 0;
}
