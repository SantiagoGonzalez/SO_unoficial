#include "primitivas.h"

void finalizar(void){
	finAnsisop();
	sigo_ejecutando=0;
}

t_puntero definirVariable(t_nombre_variable identificador_variable){
	if ((pcb->tamanioIndiceStack)==0) {
		pcb->indiceDeStack=list_create();
		t_stack* stackAux=malloc(sizeof(t_stack));
		t_variable* newVar=malloc(sizeof(t_variable));
		newVar->ID=identificador_variable;
		newVar->posicionEnMemoria.pag=pcb->paginasCodigo;
		newVar->posicionEnMemoria.offset=0;
		stackAux->vars=list_create();
		list_add(stackAux->vars,newVar);
		stackAux->cantVars+=1;
		stackAux->cantArgs=0;
		pcb->SP=list_add(pcb->indiceDeStack,stackAux);
		pcb->tamanioIndiceStack+=1;
		return (pcb->paginasCodigo-1)*tamanioPagina;
	}
	int i=0;
	uint32_t cantVarsAnt=0;
	while(i<list_size(pcb->indiceDeStack)){
		t_stack*stackAuxAnt;
		stackAuxAnt=list_get(pcb->indiceDeStack,i);
		cantVarsAnt+=stackAuxAnt->cantArgs+stackAuxAnt->cantVars;
		i++;
	}

	if((identificador_variable>='0')&&(identificador_variable<='9')){
		t_stack*stackAux;
		stackAux=list_get(pcb->indiceDeStack,pcb->SP);
		t_variable* newArg=malloc(sizeof(t_variable));
		newArg->ID=identificador_variable;
		newArg->posicionEnMemoria.pag=((cantVarsAnt*sizeof(uint32_t))/tamanioPagina)+pcb->paginasCodigo-1;
		newArg->posicionEnMemoria.offset=((cantVarsAnt*sizeof(uint32_t))%tamanioPagina);
		list_add(stackAux->args,newArg);
	//tomo de la posicion SP del indice de stack el contexto y le agrego el nuevo argumento
	}
	else{
		t_stack* stackAux;
		stackAux=list_get(pcb->indiceDeStack,pcb->SP);
		t_variable* variable=malloc(sizeof(t_variable));
		variable->ID=identificador_variable;
		variable->posicionEnMemoria.pag=((cantVarsAnt*sizeof(uint32_t))/tamanioPagina)+pcb->paginasCodigo-1;
		variable->posicionEnMemoria.offset=((sizeof(uint32_t)*cantVarsAnt)%tamanioPagina);
		list_add(stackAux->vars,variable);
		stackAux->cantVars+=1;
	}
	return (cantVarsAnt*sizeof(uint32_t))+pcb->paginasCodigo-1;
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable){
	t_stack* stackAux;
	stackAux=list_get(pcb->indiceDeStack,pcb->SP);
	t_puntero posicion= buscarVariable(stackAux,identificador_variable);
	return posicion;
}

t_valor_variable dereferenciar (t_puntero direccion_variable){
	uint32_t pagina=direccion_variable/tamanioPagina;
	uint32_t offset=direccion_variable%tamanioPagina;
	void*valor;
	valor=solicitarBytesUMC(umc,pagina,offset,4);
	t_valor_variable valorReal;
	memcpy(&valorReal,valor,sizeof(uint32_t));
	return valorReal;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor){
	uint32_t pagina=direccion_variable/tamanioPagina;
	uint32_t offset=direccion_variable%tamanioPagina;
	almacenarBytesUMC(umc,pagina,offset,4,valor);
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){
	t_valor_variable valor;
	valor=solicitarValorVarCompartida(nucleo,variable);
	return valor;
}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){

	enviarAsignarValor(variable,valor);
	return valor;
}

void irAlLabel(t_nombre_etiqueta t_nombre_etiqueta){
	pcb->PC= metadata_buscar_etiqueta(t_nombre_etiqueta,pcb->indiceDeEtiquetas.etiquetas,pcb->indiceDeEtiquetas.etiquetas_size);
	flagSaltoLinea=1;
}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	t_stack*stackAux=malloc(sizeof(t_stack));
	stackAux->args=list_create();
	stackAux->vars=list_create();
	stackAux->cantArgs=0;
	stackAux->cantVars=0;
	stackAux->retVar.pag=donde_retornar/tamanioPagina;
	stackAux->retVar.offset=donde_retornar%tamanioPagina;
	stackAux->retPos=pcb->PC;
	pcb->SP=list_add(pcb->indiceDeStack,stackAux);
	pcb->PC=metadata_buscar_etiqueta(etiqueta,pcb->indiceDeEtiquetas.etiquetas,pcb->indiceDeEtiquetas.etiquetas_size);
	flagSaltoLinea=1;

}

void retornar(t_valor_variable retorno) {
	t_stack* stackAux;
	stackAux=list_remove(pcb->indiceDeStack,pcb->SP);
	pcb->PC=stackAux->retPos;
	pcb->SP-=1;
	almacenarBytesUMC(umc,stackAux->retVar.pag,stackAux->retVar.offset,4,retorno);
	free(stackAux);
	flagSaltoLinea=1;
}

void imprimir(t_valor_variable valor_mostrar) {
	char* textoImprimir = string_itoa(valor_mostrar);
	int longitud = strlen(textoImprimir)+1;
	enviarImprimir(textoImprimir,pcb->PID,longitud);
	free(textoImprimir);
}

void imprimirTexto(char*texto) {
	int longitud= strlen(texto)+1;
	enviarImprimir(texto,pcb->PID,longitud);
}

void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo) {
	avisoEntradaSalida(nucleo,dispositivo,tiempo);
	sigo_ejecutando=0;
}

void wait(t_nombre_semaforo identificador_semaforo) {
	avisoWait(nucleo,identificador_semaforo);
}

void signal_primitiva(t_nombre_semaforo identificador_semaforo) {
	avisoSignal(nucleo,identificador_semaforo);
}
