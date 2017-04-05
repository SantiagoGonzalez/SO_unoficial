/*
 * funciones.c
 *
 *  Created on: 27/5/2016
 *      Author: utnso
 */
#include "funciones.h"
#define OVERFLOW -2
void proc_Destroy(t_nodo_lista_procesos *self) {
	free(self);
}
void pagina_Destroy(t_nodo_lista_paginas*self) {
	free(self);
}
void frame_Destroy(t_nodo_lista_frames *self) {
	free(self);
}
void entrada_TLB_Destroy(t_entrada_TLB *self) {
	free(self);
}

int encontrarPosicionEnProcesos(uint32_t pid) {
	t_nodo_lista_procesos* nodoAux;

	int i = 0;
	int encontrado = 0;
	usleep(retardoUMC*1000);
	pthread_mutex_lock(&mutexProcs);
	while ((i < list_size(listaDeProcesos)) && encontrado == 0) {
		nodoAux = list_get(listaDeProcesos, i);

		if (nodoAux->pid == pid) {
			encontrado = 1;
		} else {
			i++;
		}
	}
	pthread_mutex_unlock(&mutexProcs);
	if (encontrado == 0) {
		return -1;
	}
	return i;
}

int moverPunteroProximaPosibleVictima(int indice, uint32_t pid) {
	indice++;
	int punteroADevolver;
	t_nodo_lista_frames * nodoAux;
	int acierto = 0;
	if (indice == list_size(listaDeFrames)) {
		indice = 0;
	}
	while (indice < list_size(listaDeFrames) && acierto == 0) {
		nodoAux = list_get(listaDeFrames, indice);
		if (nodoAux->pid == pid) {
			punteroADevolver = indice;
			acierto = 1;
		}
		indice++;
		if (indice == list_size(listaDeFrames)) {
			indice = 0;
		}
	}
	return punteroADevolver;
}

int buscarVictimaClockModificado(uint32_t pid) {
	int i = encontrarPosicionEnProcesos(pid);
	int indice;
	int victima;
	int nuevoPuntero;
	int acierto = 0;
	int recorrida = 0;
	t_nodo_lista_procesos * nodoProcAux;
	t_nodo_lista_frames * nodoFrameAux;
	int referenciaDeVuelta;
	pthread_mutex_lock(&mutexProcs);
	nodoProcAux = list_get(listaDeProcesos, i);
	indice = nodoProcAux->indiceRecorridaClock;
	referenciaDeVuelta = nodoProcAux->indiceRecorridaClock;
	pthread_mutex_unlock(&mutexProcs);
	pthread_mutex_lock(&mutexMarcos);
	while (indice < list_size(listaDeFrames) && acierto == 0) {
		nodoFrameAux = list_get(listaDeFrames, indice);
		if (nodoFrameAux->pid == pid && recorrida == 0) {
			if (nodoFrameAux->bitReferencia == 0
					&& nodoFrameAux->bitModificado == 0) {
				victima = indice;
				nuevoPuntero = moverPunteroProximaPosibleVictima(indice, pid); //
				nodoProcAux->indiceRecorridaClock = nuevoPuntero;
				acierto = 1;
			}
		} else if (nodoFrameAux->pid == pid && recorrida == 1) {
			if (nodoFrameAux->bitReferencia == 0
					&& nodoFrameAux->bitModificado == 1) {
				victima = indice;
				nuevoPuntero = moverPunteroProximaPosibleVictima(indice, pid); //
				nodoProcAux->indiceRecorridaClock = nuevoPuntero;
				acierto = 1;
			}
			nodoFrameAux->bitReferencia = 0;
		}
		indice++;
		if (indice == list_size(listaDeFrames)) {
			indice = 0;
		}
		if ((recorrida == 0) && (indice == referenciaDeVuelta)) {
			recorrida = 1;
		} else if ((recorrida == 1) && (indice == referenciaDeVuelta)) {
			recorrida = 0;
		}
	}
	pthread_mutex_unlock(&mutexMarcos);
	return victima;
}
void actualizarBitReferencia(uint32_t id_frame) {
	int i = 0;
	int acierto = 0;
	t_nodo_lista_frames*nodoaux;
	pthread_mutex_lock(&mutexMarcos);
	while (i < list_size(listaDeFrames) && acierto == 0) {
		nodoaux = list_get(listaDeFrames, i);
		if (nodoaux->id_frame == id_frame) {
			nodoaux->bitReferencia = 1;
			//list_replace_and_destroy_element(listaDeFrames, i, nodoaux,
			//	(void*) frame_Destroy);
			acierto = 1;
		}
		i++;
	}
	pthread_mutex_unlock(&mutexMarcos);
}
void actualizarBitModificado(uint32_t id_frame) {
	int i = 0;
	int acierto = 0;
	t_nodo_lista_frames*nodoaux;
	pthread_mutex_lock(&mutexMarcos);
	while (i < list_size(listaDeFrames) && acierto == 0) {
		nodoaux = list_get(listaDeFrames, i);
		if (nodoaux->id_frame == id_frame) {
			nodoaux->bitModificado = 1;
			//list_replace_and_destroy_element(listaDeFrames, i, nodoaux,
			//	(void*) frame_Destroy);
			acierto = 1;
		}
		i++;
	}
	pthread_mutex_unlock(&mutexMarcos);
}

void * lecturaMemoria(uint32_t frame, uint32_t offset, uint32_t tamanio) {
	void * bytes = malloc(tamanio);
	int posicion = (int) frame * tamanioFrame + offset;

	usleep(retardoUMC * 1000);
	pthread_mutex_lock(&mutexMP);
	void * posicionAux = memoriaprincipal + posicion;
	memcpy(bytes, posicionAux, tamanio);
	pthread_mutex_unlock(&mutexMP);
	pthread_mutex_lock(&mutexAccesoMem);
	accesosAMemoria++;
	pthread_mutex_unlock(&mutexMP);
	actualizarBitReferencia(frame);
	return bytes;
}

void escrituraMemoria(uint32_t frame, uint32_t offset, uint32_t tamanio,
		void * buffer) {
	int posicion = (int) frame * tamanioFrame + offset;
	usleep(retardoUMC * 1000);
	pthread_mutex_lock(&mutexMP);
	void * posicionAux = memoriaprincipal + posicion;
	memcpy(posicionAux, buffer, tamanio);
	pthread_mutex_unlock(&mutexMP);
	pthread_mutex_lock(&mutexAccesoMem);
	accesosAMemoria++;
	pthread_mutex_unlock(&mutexAccesoMem);
	actualizarBitReferencia(frame);
	free(buffer);
}

void actualizarBitUltimoAccesoTLB(uint32_t pid, int nroFrame) {
	t_entrada_TLB* nodoAux;
	int i = 0;
	int acierto = 0;

	pthread_mutex_lock(&mutexTLB);
	while (i < list_size(tlb) && acierto == 0) {
		nodoAux = list_get(tlb, i);
		if (nodoAux->pid == pid && nodoAux->id_frame == (uint32_t) nroFrame) {
			pthread_mutex_lock(&mutexAccesoMem);
			nodoAux->ultimo_acceso = accesosAMemoria;
			pthread_mutex_unlock(&mutexAccesoMem);
			acierto = 1;
		}
		i++;
	}
	pthread_mutex_unlock(&mutexTLB);

}

void cambiarRetardo(int nuevoRetardo) {
	pthread_mutex_lock(&mutexRetardo);
	retardoUMC = nuevoRetardo;
	pthread_mutex_unlock(&mutexRetardo);
}

void flushMemoria() {
	int i = 0;
	t_nodo_lista_frames *nodoAux;
	pthread_mutex_lock(&mutexMarcos);
	while (i < list_size(listaDeFrames)) {
		nodoAux = list_get(listaDeFrames, i);
		nodoAux->bitModificado = 1;
		//list_replace_and_destroy_element(listaDeFrames,i,nodoAux,(void*)frame_Destroy);
		i++;
	}
	pthread_mutex_unlock(&mutexMarcos);
}

void flushTLB() {
	t_entrada_TLB* aux;
	int i = 0;
	pthread_mutex_lock(&mutexTLB);
	while (i < list_size(tlb)) {
		aux = list_get(tlb, i);
		aux->pid = 0;
		i++;
	}

	pthread_mutex_unlock(&mutexTLB);
}

void limpiarEntradasTLBporFrame(uint32_t frame) {
	t_entrada_TLB* entradaAux;
	int i = 0;
	pthread_mutex_lock(&mutexTLB);
	while (i < list_size(tlb)) {
		entradaAux = list_get(tlb, i);
		if (entradaAux->id_frame == frame) {
			entradaAux->pid = 0;
			//list_remove_and_destroy_element(tlb,i,(void*)entrada_TLB_Destroy);
		}
		i++;
	}
	pthread_mutex_unlock(&mutexTLB);
}

void limpiarEntradasTLB(uint32_t pid) {
	t_entrada_TLB* entradaAux;
	int i = 0;
	pthread_mutex_lock(&mutexTLB);
	while (i < list_size(tlb)) {
		entradaAux = list_get(tlb, i);
		if (entradaAux->pid == pid) {
			entradaAux->pid = 0;
			//list_remove_and_destroy_element(tlb,i,(void*)entrada_TLB_Destroy);
		}
		i++;
	}
	pthread_mutex_unlock(&mutexTLB);
}

//void clock(uint32_t pid, uint32_t numPagina)

void liberarMarcos(uint32_t pid) {
	t_nodo_lista_frames* nodoAux;
	int i = 0;
	pthread_mutex_lock(&mutexMarcos);
	while (i < list_size(listaDeFrames)) {
		nodoAux = list_get(listaDeFrames, i);
		if (nodoAux->pid == pid) {
			nodoAux->pid = 0;
			//list_replace(listaDeFrames, i, &nodoAux);
		}
		i++;
	}
	pthread_mutex_unlock(&mutexMarcos);

}

int buscarEnListaProcesos(uint32_t pid, int numPagina) {
	uint32_t nroframe;
	t_nodo_lista_procesos* entradaAux;
	t_nodo_lista_paginas* paginaAux;

	int i = 0;
	int j = 0;
	int acierto = 0;
	int encontroPagina = 0;
	usleep(retardoUMC*1000);
	pthread_mutex_lock(&mutexProcs);
	while (i < list_size(listaDeProcesos) && acierto == 0) {
		entradaAux = list_get(listaDeProcesos, i);
		if (entradaAux->pid == pid) {
			while (j < list_size(entradaAux->lista_paginas) && acierto == 0) {
				paginaAux = list_get(entradaAux->lista_paginas, j);
				if (paginaAux->nro_pagina == numPagina) {
					encontroPagina = 1;
					if (paginaAux->status == 'M') {
						nroframe = paginaAux->id_frame;

						acierto = 1;
					} else {
						pthread_mutex_unlock(&mutexProcs);
						return -1;
					}
				} //FIN IF
				j++;
			} //FIN WHILE

		} //FIN IF PID
		i++;
	}
	pthread_mutex_unlock(&mutexProcs);
	if (encontroPagina == 0) {
		return -2;
	}
	return nroframe;
}

int buscarenTLB(uint32_t pid, int numPag) {
	int nroframe = -1;
	t_entrada_TLB* entradaAux;
	int i = 0;
	int acierto = 0;
	pthread_mutex_lock(&mutexTLB);
	while (i < list_size(tlb) && acierto == 0) {
		entradaAux = list_get(tlb, i);
		if ((entradaAux->pid == pid)
				&& (entradaAux->nro_pagina == (uint32_t) numPag)) {
			nroframe = (int) (entradaAux->id_frame);
			acierto = 1;
		}
		i++;
	}
	pthread_mutex_unlock(&mutexTLB);
	return nroframe;
}

/*void*lecturaMemoria(uint32_t frame, int offset, int tamanio) {
 void*bytes = malloc(tamanio);
 int posicion = (int) frame * tamanioFrame + offset;
 usleep(retardoUMC * 1000);
 actualizarBitReferencia(frame);
 pthread_mutex_lock(&mutexMP);
 void*posicionAuxiliar = memoriaprincipal + posicion;
 memcpy(bytes, posicionAuxiliar, tamanio);
 pthread_mutex_unlock(&mutexMP);
 return bytes;

 }

 void escrituraMemoria(uint32_t nroframe, int offset, int tamanioPag,
 void*buffer) {
 int posicion = (int) nroframe * tamanioFrame + offset;
 usleep(retardoUMC * 1000);
 pthread_mutex_lock(&mutexMP);
 memcpy(memoriaprincipal + posicion, buffer, sizeof(buffer));
 actualizarBitReferencia(nroframe);
 actualizarBitModificado(nroframe);
 pthread_mutex_unlock(&mutexMP);
 }*/

int buscarEntradaMasAntigua() {
	t_entrada_TLB * entradaAux;
	int i = 0;
	int j = 0;
	int ultimo_acceso_auxiliar;
	pthread_mutex_lock(&mutexAccesoMem);
	ultimo_acceso_auxiliar = (int) accesosAMemoria;
	pthread_mutex_unlock(&mutexAccesoMem);
	while (i < list_size(tlb)) {
		entradaAux = list_get(tlb, i);
		if ((entradaAux->ultimo_acceso) < ultimo_acceso_auxiliar) {
			ultimo_acceso_auxiliar = entradaAux->ultimo_acceso;
			j = i;
		}
		i++;
	}
	return j;
}

void LRU(uint32_t paginaNueva, uint32_t pid, uint32_t frameNuevo) {

	int indice_entrada;
	indice_entrada = buscarEntradaMasAntigua();
	t_entrada_TLB * entradaAux;
	entradaAux = list_get(tlb, indice_entrada);
	entradaAux->nro_pagina = paginaNueva;
	entradaAux->id_frame = frameNuevo;
	entradaAux->pid = pid;
	entradaAux->ultimo_acceso = accesosAMemoria;
	//list_replace_and_destroy_element(tlb, indice_entrada, &entradaAux,
	//	(void*) entrada_TLB_Destroy);

}

int cantidadFramesDisponibles() {
	int i = 0;
	int contador = 0;
	t_nodo_lista_frames* nodoAux;
	pthread_mutex_lock(&mutexMarcos);
	while (i < list_size(listaDeFrames)) {
		nodoAux = list_get(listaDeFrames, i);
		if (nodoAux->pid == 0) {
			contador++;
		}
		i++;
	}
	pthread_mutex_unlock(&mutexMarcos);
	return contador;
}

int buscarVictimaClock(uint32_t pid) {
	int i = encontrarPosicionEnProcesos(pid);
	int indice;
	int victima;
	int nuevoPuntero;
	int acierto = 0;
	t_nodo_lista_procesos * nodoProcAux;
	t_nodo_lista_frames * nodoFrameAux;
	pthread_mutex_lock(&mutexProcs);
	nodoProcAux = list_get(listaDeProcesos, i);
	pthread_mutex_unlock(&mutexProcs);
	indice = nodoProcAux->indiceRecorridaClock;
	pthread_mutex_lock(&mutexMarcos);
	while (indice < list_size(listaDeFrames) && acierto == 0) {
		nodoFrameAux = list_get(listaDeFrames, indice);
		if (nodoFrameAux->pid == pid) {
			if (nodoFrameAux->bitReferencia == 0) {
				victima = indice;
				nuevoPuntero = moverPunteroProximaPosibleVictima(indice, pid);//
				nodoProcAux->indiceRecorridaClock = nuevoPuntero;
				acierto = 1;
			} else if (nodoFrameAux->bitReferencia == 1) {
				nodoFrameAux->bitReferencia = 0;
			}

		}
		indice++;
		if (indice == list_size(listaDeFrames)) {
			indice = 0;
		}
	}
	pthread_mutex_unlock(&mutexMarcos);
	return victima;
}
void realizarCambiosEnPaginaVieja(uint32_t frame, int indiceproc) {
	t_nodo_lista_procesos* nodoAux;
	t_nodo_lista_paginas*nodoPagAux;
	int i = 0;
	int acierto = 0;
	pthread_mutex_lock(&mutexProcs);
	nodoAux = list_get(listaDeProcesos, indiceproc);
	while (i < list_size(nodoAux->lista_paginas) && acierto == 0) {
		nodoPagAux = list_get(nodoAux->lista_paginas, i);
		if (nodoPagAux->id_frame == frame) {
			nodoPagAux->status = 'S';
			acierto = 1;
		}
		i++;
	}
	pthread_mutex_unlock(&mutexProcs);
}
void realizarCambiosEnPaginaNueva(uint32_t pagNueva, uint32_t frame, int indiceproc) {
	t_nodo_lista_procesos* nodoAux;
	t_nodo_lista_paginas*nodoPagAux;
	int i = 0;
	int acierto = 0;
	pthread_mutex_lock(&mutexProcs);
	nodoAux = list_get(listaDeProcesos, indiceproc);
	while (i < list_size(nodoAux->lista_paginas) && acierto == 0) {
		nodoPagAux = list_get(nodoAux->lista_paginas, i);
		if (nodoPagAux->nro_pagina == pagNueva) {
			nodoPagAux->id_frame = frame;
			nodoPagAux->status = 'M';
			acierto = 1;
		}
		i++;
	}
	pthread_mutex_unlock(&mutexProcs);
}

void enviarPaginaAlSwap(uint32_t pid, int numPag, void*buffer) {
	int pidAMandar = (int) pid;
	int orden = ESCRIBIR;
	int cantidadPaginasCodigo = 0;
	int respuestaSend;
		int puntero = 0;
		void*bufferaEnviar = malloc((sizeof(int) * 4) + (tamanioFrame));
		memcpy(bufferaEnviar, (void*) &pidAMandar, sizeof(int));
		puntero += sizeof(int);
		memcpy(bufferaEnviar + puntero, &orden, sizeof(int));
		puntero += sizeof(int);
		memcpy(bufferaEnviar + puntero, &cantidadPaginasCodigo, sizeof(int));
		puntero += sizeof(int);
		memcpy(bufferaEnviar + puntero, &numPag, sizeof(int));
		puntero += sizeof(int);
		memcpy(bufferaEnviar + puntero, buffer, tamanioFrame);
		puntero += tamanioFrame;
		respuestaSend=send(socketSwap, &puntero, sizeof(int), 0);
		if(respuestaSend<0){
				log_error(logger,"se desconecto el swap,abortando la umc");
				abort();
			}
		respuestaSend=send(socketSwap, bufferaEnviar, puntero, 0);
		if(respuestaSend<0){
				log_error(logger,"se desconecto el swap,abortando la umc");
				abort();
			}
		free(buffer);
		free(bufferaEnviar);
	}

void indicarPaginaQueVaAlSwapyEnviar(uint32_t id_frame, int indiceproc,uint32_t bitDeModificado,uint32_t pid) {
	t_nodo_lista_paginas* nodoPagAux;
	t_nodo_lista_procesos* nodoAuxProc;
	int i = 0;
	int acierto = 0;
	pthread_mutex_lock(&mutexProcs);
	nodoAuxProc = list_get(listaDeProcesos, indiceproc);
	while (i < list_size(nodoAuxProc->lista_paginas) && acierto == 0) {
		nodoPagAux = list_get(nodoAuxProc->lista_paginas, i);
		if (nodoPagAux->id_frame == id_frame) {
			nodoPagAux->status = 'S';
			acierto = 1;
		}
		i++;
	}
	pthread_mutex_unlock(&mutexProcs);
	if (bitDeModificado==1){
		void*buffer=malloc(tamanioFrame);
		buffer=lecturaMemoria(id_frame,0,tamanioFrame);
		pthread_mutex_lock(&mutexSwap);
		enviarPaginaAlSwap(pid,nodoPagAux->nro_pagina,buffer);
		pthread_mutex_unlock(&mutexSwap);
	}
}
void algoritmoDeReemplazo(uint32_t pid, uint32_t pagNueva, void *codigo,int*id_frame) {
	int indiceVictima;
	if (strcmp(algoritmo, "CLOCK") == 0) {
		indiceVictima = buscarVictimaClock(pid);
	} else if (strcmp(algoritmo, "CLOCK-M")) {
		indiceVictima = buscarVictimaClockModificado(pid);
	}
	int indiceproc = encontrarPosicionEnProcesos(pid);
	t_nodo_lista_frames* nodoAux;
	pthread_mutex_lock(&mutexMarcos);
	nodoAux = list_get(listaDeFrames, indiceVictima);
	*id_frame =(int)(nodoAux->id_frame);
	uint32_t id_frame_casteado=(uint32_t)(*id_frame);
	if (nodoAux->bitModificado == 1) {
		indicarPaginaQueVaAlSwapyEnviar(id_frame_casteado,indiceproc,nodoAux->bitModificado,pid);
		nodoAux->bitModificado=0;
		//enviarPaginaViejaAlSwap(id_frame,pid);
	}
	pthread_mutex_unlock(&mutexMarcos);
	realizarCambiosEnPaginaVieja(id_frame_casteado,indiceproc);
	limpiarEntradasTLBporFrame(id_frame_casteado);
	realizarCambiosEnPaginaNueva(pagNueva, id_frame_casteado, indiceproc);
	escrituraMemoria(id_frame_casteado, 0, tamanioFrame, codigo);
}

/*void inicializarPrograma(uint32_t idPrograma, int paginasRequeridas,
 char * codigoPrograma, int socketNucleo) {

 int largoPrograma = strlen(codigoPrograma) + 1; //?
 int paginasCodigo = largoPrograma / size_frames
 + largoPrograma % size_frames; //not sure
 int respuestaInicializacion;

 pthread_mutex_lock(&mutexSwap);
 enviarCodigoASwap(socketSwap, paginasRequeridas, idPrograma, paginasCodigo);
 log_info(logger, "Se envió nuevo programa pid %d a Swap", idPrograma);
 //enviarPaginas(enviar pagina x pagina)
 respuestaInicializacion = recibirRespuestaInicializacion(socketSwap);
 pthread_mutex_unlock(&mutexSwap);

 if(respuestaInicializacion == inicioProgramaExito){
 enviarRespuestaInicializacionExito(socketNucleo);
 log_info(logger, "Se inicializó nuevo programa pid %d", idPrograma);
 }
 else {
 enviarRespuestaInicializacionError(socketNucleo);
 log_info(logger, "No se pudo inicializar programa pid %d. No hay espacio en Swap", idPrograma);
 return;
 }

 //aca tengo que crear un puntero o una estructura?
 t_nodo_lista_procesos* unNodo = malloc(sizeof(t_nodo_lista_procesos));
 unNodo->pid = idPrograma;
 unNodo->cantPaginas = paginasRequeridas;
 //unNodo.framesAsignados = 0;
 unNodo->lista_paginas = list_create();
 //unNodo.punteroClock = -1;
 int i;
 for (i = 0; i < paginasRequeridas; i++) {
 t_nodo_lista_paginas* unaPagina = malloc(sizeof(t_nodo_lista_paginas));
 unaPagina->nro_pagina = i;
 unaPagina->status = 'S';
 list_add(unNodo->lista_paginas, unaPagina);
 }

 pthread_mutex_lock(&mutexProcesos);
 list_add(listaProcesos, &unNodo);
 pthread_mutex_unlock(&mutexProcesos);

 free(codigoPrograma);

 }*/

void cargarenTLB(uint32_t pid, int numPagina, uint32_t nroFrame) {
	t_entrada_TLB* tlbAux;
	int i = 0;
	int acierto = 0;
	pthread_mutex_lock(&mutexTLB);
	while (i < list_size(tlb) && acierto == 0) {
		tlbAux = list_get(tlb, i);
		if (tlbAux->pid == 0) {
			tlbAux->id_frame = nroFrame;
			tlbAux->nro_pagina = numPagina;
			tlbAux->pid = pid;
			pthread_mutex_lock(&mutexAccesoMem);
			tlbAux->ultimo_acceso = accesosAMemoria;
			pthread_mutex_unlock(&mutexAccesoMem);
			acierto = 1;
		}
		i++;
	}
	if (acierto == 0) {
		LRU(numPagina, pid, nroFrame);
	}
	pthread_mutex_unlock(&mutexTLB);
}
int cargarPaginaenMemoria(uint32_t pid, int numPag, void*buffer,int*id_frame) {
	t_nodo_lista_frames * framesAux;
	t_nodo_lista_procesos* procAux;
	int i = 0;
	int j = 0;
	int permitido = 0;
	int acierto = 0;
	int aciertoFrameLibre = 0;
	int cantDisponibles;
	pthread_mutex_lock(&mutexProcs);
	while (i < list_size(listaDeProcesos) && acierto == 0) {
		procAux = list_get(listaDeProcesos, i);
		if (procAux->pid == pid) {
			if (procAux->cantFramesAsignados < marcos_x_proc) {
				if (procAux->cantFramesAsignados == 0) {
					cantDisponibles = cantidadFramesDisponibles();
				}
				permitido = 1;
			}
			acierto = 1;
		}
		i++;
	}
	pthread_mutex_unlock(&mutexProcs);
	pthread_mutex_lock(&mutexMarcos);
	uint8_t cantAuxFramesAsignados=procAux->cantFramesAsignados;
	pthread_mutex_unlock(&mutexMarcos);
	if (cantDisponibles == 0 && cantAuxFramesAsignados == 0) {
		//avisarNucleoFinPrograma
		//avisarCPUFinPrograma
		//avisarSwapFinPrograma
		return -1;
	}
	if (permitido == 1) {
		pthread_mutex_lock(&mutexMarcos);
		while (j < list_size(listaDeFrames) && aciertoFrameLibre == 0) {
			framesAux = list_get(listaDeFrames, j);
			if (framesAux->pid == 0) {
				framesAux->pid = pid;
				framesAux->bitReferencia = 1;
				framesAux->bitModificado = 0;
				*id_frame=framesAux->id_frame;
				realizarCambiosEnPaginaNueva(numPag,j,i-1);
				break;
			}
			j++;
		}
		pthread_mutex_unlock(&mutexMarcos);
		pthread_mutex_lock(&mutexProcs);
				procAux->cantFramesAsignados++;
				if(procAux->cantFramesAsignados==1){
				procAux->indiceRecorridaClock=j;
				}
		pthread_mutex_unlock(&mutexProcs);
		escrituraMemoria(framesAux->id_frame, 0, tamanioFrame, buffer);
	} else {
		algoritmoDeReemplazo(pid, numPag, buffer,id_frame);
	}
	return 1;
}
int inicializarEnSwap(int socketSwap, int cantidadPaginasCodigo, uint32_t pid) {
	int respuestaSwap;
	int pidAMandar = (int) pid;
	int orden = INICIAR;
	int puntero = 0;
	char*mensaje = "";
	void*buffer = malloc(sizeof(int) * 4 + (strlen(mensaje) + 1));
	memcpy(buffer, (void*) &pidAMandar, sizeof(int));
	puntero += sizeof(int);
	memcpy(buffer + puntero, &orden, sizeof(int));
	puntero += sizeof(int);
	memcpy(buffer + puntero, &cantidadPaginasCodigo, sizeof(int));
	puntero += sizeof(int);
	int numpag = 0;
	memcpy(buffer + puntero, &numpag, sizeof(int));
	puntero += sizeof(int);
	memcpy(buffer + puntero, mensaje, strlen(mensaje) + 1);
	puntero += (strlen(mensaje) + 1);
	int respuestaSend;
	respuestaSend=send(socketSwap, &puntero, sizeof(int), 0);
	if(respuestaSend<0){
		log_error(logger,"se desconecto el swap,abortando la umc");
		abort();
	}
	respuestaSend=send(socketSwap, buffer, puntero, 0);
	if(respuestaSend<0){
			log_error(logger,"se desconecto el swap,abortando la umc");
			abort();
		}
	int bytesRecibidos;
	bytesRecibidos=recv(socketSwap, &respuestaSwap, sizeof(int), 0);
	if(bytesRecibidos<=0){
		log_error(logger,"se desconecto el swap, abortando la umc");
		abort();
	}
	return respuestaSwap;
	free(buffer);

}

void pedirAlSwap(int numPagina, uint32_t pid, void*bufferRecepcion) {
	//int tamanioAMandar=0;
	int pidAMandar = (int) pid;
	int orden = LEER;
	int puntero = 0;
	int cantidadPaginasCodigo = 0;
	char*mensaje = "";
	void*buffer = malloc((sizeof(int) * 4) + (strlen(mensaje) + 1));
	memcpy(buffer, (void*) &pidAMandar, sizeof(int));
	puntero += sizeof(int);
	memcpy(buffer + puntero, &orden, sizeof(int));
	puntero += sizeof(int);
	memcpy(buffer + puntero, &cantidadPaginasCodigo, sizeof(int));
	puntero += sizeof(int);
	memcpy(buffer + puntero, &numPagina, sizeof(int));
	puntero += sizeof(int);
	memcpy(buffer + puntero, mensaje, strlen(mensaje) + 1);
	puntero += (strlen(mensaje) + 1);
	int respuestaSend;
	respuestaSend=send(socketSwap, &puntero, sizeof(int), 0);
	if(respuestaSend<0){
		log_error(logger,"se desconecto el swap,abortando la umc");
		abort();
	}
	send(socketSwap, buffer, puntero, 0);
	free(buffer);
	int bytesRecibidos;
	bytesRecibidos=recv(socketSwap, bufferRecepcion, tamanioFrame, 0);
	if(bytesRecibidos<=0){
		log_error(logger,"se desconecto el swap,abortando la umc");
		abort();
	}
}
void liberarPaginas(int indice) {
	t_nodo_lista_procesos* nodoAux;
	nodoAux = list_get(listaDeProcesos, indice);
	list_clean_and_destroy_elements(nodoAux->lista_paginas,
			(void*) pagina_Destroy);

}
void dumpearEstructuraPaginas(t_nodo_lista_procesos*nodoAux, FILE*archivo) {
	int i = 0;
	t_nodo_lista_paginas*nodoAuxPagina;
	printf("Numero de Pagina\tEstado\n");
	fprintf(archivo, "Numero de Pagina\tEstado\n");
	//pthread_mutex_lock(&mutexProcs);
	while (i < list_size(nodoAux->lista_paginas)) {
		nodoAuxPagina = list_get(nodoAux->lista_paginas, i);
		printf("%d               \t%c\n", nodoAuxPagina->nro_pagina,
				nodoAuxPagina->status);
		fprintf(archivo, "%d               \t%c\n", nodoAuxPagina->nro_pagina,
				nodoAuxPagina->status);
		i++;
	}
	//pthread_mutex_unlock(&mutexProcs);
}
void dumpPIDparaDumpTotal(t_nodo_lista_procesos*nodoAux, FILE*archivo) {
	fprintf(archivo,
			"PID\tCantidad de Frames Asignados\tCantidad de Paginas\n");
	printf("PID\tCantidad de Frames Asignados\tCantidad de Paginas\n");
	printf("%d\t%d                            \t%d\n", nodoAux->pid,
			nodoAux->cantFramesAsignados, nodoAux->cantPaginas);
	fprintf(archivo, "%d\t%d                            \t%d\n", nodoAux->pid,
			nodoAux->cantFramesAsignados, nodoAux->cantPaginas);
	dumpearEstructuraPaginas(nodoAux, archivo);
}
void dumpTotal() {
	FILE*archivo = fopen("dumpUMC.txt", "w+");
	int i = 0;
	t_nodo_lista_procesos*nodoAux;
	pthread_mutex_lock(&mutexProcs);
	while (i < list_size(listaDeProcesos)) {
		nodoAux = list_get(listaDeProcesos, i);
		dumpPIDparaDumpTotal(nodoAux, archivo);
		i++;
	}
	pthread_mutex_unlock(&mutexProcs);

	printf("\nDump de Memoria Principal\n");
	fprintf(archivo, "\nDump de Memoria Principal\n");
	pthread_mutex_lock(&mutexMP);
	hexdump(memoriaprincipal, tamanioTotalmemoria, archivo);
	pthread_mutex_unlock(&mutexMP);
	fclose(archivo);
}

void dumpMemoriaPID(t_nodo_lista_procesos*nodoAux, FILE*archivo) {
	int i = 0;
	t_nodo_lista_paginas*nodoAuxPagina;
	printf("\nDump memoria del PID:\n");
	fprintf(archivo, "\nDump memoria del PID:\n");
	pthread_mutex_lock(&mutexProcs);
	while (i < list_size(nodoAux->lista_paginas)) {
		nodoAuxPagina = list_get(nodoAux->lista_paginas, i);
		if (nodoAuxPagina->status == 'M') {
			void*buffer = lecturaMemoria(nodoAuxPagina->id_frame, 0,
					tamanioFrame);
			hexdump(buffer, tamanioFrame, archivo);
			free(buffer);
		}
		i++;
	}
	pthread_mutex_unlock(&mutexProcs);
}
void dumpPID(uint32_t pid) {
	int indiceProceso = encontrarPosicionEnProcesos(pid);
	if (indiceProceso == -1) {
		printf("PID no valido\n");
		return;
	}
	FILE* archivo = fopen("dumpPID.txt", "w+");
	fprintf(archivo,
			"PID\tCantidad de Frames Asignados\tCantidad de Paginas\n");
	printf("PID\tCantidad de Frames Asignados\tCantidad de Paginas\n");
	pthread_mutex_lock(&mutexProcs);
	t_nodo_lista_procesos*nodoAux = list_get(listaDeProcesos, indiceProceso);
	pthread_mutex_unlock(&mutexProcs);
	printf("%d\t%d                            \t%d\n", pid,
			nodoAux->cantFramesAsignados, nodoAux->cantPaginas);
	fprintf(archivo, "%d\t%d                            \t%d\n", pid,
			nodoAux->cantFramesAsignados, nodoAux->cantPaginas);
	pthread_mutex_lock(&mutexProcs);
	dumpearEstructuraPaginas(nodoAux, archivo);
	pthread_mutex_unlock(&mutexProcs);
	dumpMemoriaPID(nodoAux, archivo);
	fclose(archivo);
}
/*void chequearPedidoValido(uint32_t pid, int numPagina){
 int indiceProc=encontrarPosicionEnProcesos(pid);
 }*/ //Preguntar a sofia que carajo hace esto
void enviarCodigoAlSwap(uint32_t pid, int cantPaginas, char*codPrograma) {
	int i;
	uint32_t tamanioAMandar = strlen(codPrograma) ;
	uint32_t pidAMandar =  pid;
	uint32_t orden = ESCRIBIR;
	uint32_t cantidadPaginasCodigo = 0;
	uint32_t punteroPrograma = 0;
	int respuestaSend;
	for (i = 0; i < cantPaginas; i++) {
		int puntero = 0;
		void*buffer;
		if(tamanioAMandar<tamanioFrame){
		buffer = malloc((sizeof(uint32_t) * 4) + (tamanioAMandar));
		memcpy(buffer, (void*) &pidAMandar, sizeof(uint32_t));
		puntero += sizeof(uint32_t);
		memcpy(buffer + puntero, &orden, sizeof(uint32_t));
		puntero += sizeof(uint32_t);
		memcpy(buffer + puntero, &cantidadPaginasCodigo, sizeof(uint32_t));
		puntero += sizeof(uint32_t);
		memcpy(buffer + puntero, &i, sizeof(uint32_t));
		puntero += sizeof(uint32_t);
			memcpy(buffer + puntero, &codPrograma[punteroPrograma], tamanioAMandar);
			puntero += tamanioAMandar;
		}
		else{
			buffer = malloc((sizeof(uint32_t) * 4) + (tamanioFrame));
			memcpy(buffer, (void*) &pidAMandar, sizeof(uint32_t));
			puntero += sizeof(uint32_t);
			memcpy(buffer + puntero, &orden, sizeof(uint32_t));
			puntero += sizeof(uint32_t);
			memcpy(buffer + puntero, &cantidadPaginasCodigo, sizeof(uint32_t));
			puntero += sizeof(uint32_t);
			memcpy(buffer + puntero, &i, sizeof(uint32_t));
			puntero += sizeof(uint32_t);
		memcpy(buffer + puntero, &codPrograma[punteroPrograma], tamanioFrame);
		puntero += tamanioFrame;
		}
		respuestaSend=send(socketSwap, &puntero, sizeof(uint32_t), 0);
		if(respuestaSend<0){
				log_error(logger,"se desconecto el swap,abortando la umc");
				abort();
			}
		respuestaSend=send(socketSwap, buffer, puntero, 0);
		if(respuestaSend<0){
				log_error(logger,"se desconecto el swap,abortando la umc");
				abort();
			}
		free(buffer);
		punteroPrograma += tamanioFrame;
		tamanioAMandar -= tamanioFrame;
	}
}
void inicializarPrograma(uint32_t pid, int cantPaginas, char* codPrograma) {
	int largocodigo = strlen(codPrograma) + 1;
	int avisoNucleo;
	int cantidadPaginasCodigo = largocodigo / tamanioFrame;
	if ((largocodigo % tamanioFrame) > 0) {
		cantidadPaginasCodigo++;
	}
	//MUTEX_SWAP

	pthread_mutex_lock(&mutexSwap);
	int validacionSwap = inicializarEnSwap(socketSwap, cantidadPaginasCodigo,
			pid);
	if (validacionSwap == 1) {
		pthread_mutex_unlock(&mutexSwap);
		//aviso que no pude iniciar el programa;
		avisoNucleo = PROGRAMA_NO_INICIALIZADO;
		send(socketNucleo, &avisoNucleo, sizeof(int), 0);
		log_error(logger, "No se pudo inicializar el programa con pid: %d",
				pid);
		return;
	} else {
		//aviso que pude inicializar al NUCLEO
		avisoNucleo = PROGRAMA_INICIALIZADO;
		send(socketNucleo, &avisoNucleo, sizeof(int), 0);
		enviarCodigoAlSwap(pid, cantidadPaginasCodigo, codPrograma);
		t_nodo_lista_procesos* unNodo = malloc(sizeof(t_nodo_lista_procesos));
		unNodo->pid = pid;
		unNodo->cantPaginas = cantPaginas;
		unNodo->lista_paginas = list_create();
		unNodo->cantFramesAsignados = 0;
		//asignarFrames(pid,cantPaginas);
		int i;
		for (i = 0; i < cantPaginas; i++) {
			t_nodo_lista_paginas * unNodoPag = malloc(
					sizeof(t_nodo_lista_paginas));
			unNodoPag->id_frame=cantidadDeFrames+1;
			unNodoPag->nro_pagina = i;
			unNodoPag->status = 'S';
			list_add(unNodo->lista_paginas, unNodoPag);

		}
		pthread_mutex_lock(&mutexProcs);
		list_add(listaDeProcesos, unNodo);
		pthread_mutex_unlock(&mutexProcs);
		log_info(logger,
				"Se inicializo correctamente el programa correspondiente al PID: %d",
				pid);
	}
}

void avisarSwapFinPrograma(uint32_t pid) {
	//int respuestaSwap;
	int pidAMandar = (int) pid;
	int orden = FINALIZAR;
	int puntero = 0;
	int cantidad = 0;
	char*mensaje = "";
	void*buffer = malloc(sizeof(int) * 4 + (strlen(mensaje) + 1));
	memcpy(buffer, (void*) &pidAMandar, sizeof(int));
	puntero += sizeof(int);
	memcpy(buffer + puntero, &orden, sizeof(int));
	puntero += sizeof(int);
	memcpy(buffer + puntero, &cantidad, sizeof(int));
	puntero += sizeof(int);
	int numpag = 0;
	memcpy(buffer + puntero, &numpag, sizeof(int));
	memcpy(buffer + puntero, mensaje, strlen(mensaje) + 1);
	puntero += (strlen(mensaje) + 1);
	int respuestasend;
	respuestasend=send(socketSwap, &puntero, sizeof(int), 0);
	if(respuestasend<0){
		log_error(logger,"Se desconecto el swap");
		abort();
	}
	send(socketSwap, buffer, puntero, 0);
	//recv(socketSwap,&respuestaSwap,sizeof(int),0);
	free(buffer);
	log_info(logger, "Aviso al swap que termina el programa con pid: %d", pid);
}

void avisarNucleoFinProgramaPorOverFlow(uint32_t pid) {
	int mensaje = ERROR_OVERFLOW;
	int pidAEnviar = (int) pid;
	int tamanio = sizeof(int) * 2;
	void*buffer = malloc(sizeof(int) * 2);
	memcpy(buffer, &pidAEnviar, sizeof(int));
	memcpy(buffer + sizeof(int), &mensaje, sizeof(int));
	send(socketNucleo, buffer, tamanio, 0);
	free(buffer);
}
void avisarNucleoFinPrograma(uint32_t pid) {
	int mensaje = FINALIZAR_PROGRAMA_UMC;
	int pidAEnviar = (int) pid;
	int tamanio = sizeof(int) * 2;
	void*buffer = malloc(sizeof(int) * 2);
	memcpy(buffer, &pidAEnviar, sizeof(int));
	memcpy(buffer + sizeof(int), &mensaje, sizeof(int));
	send(socketNucleo, buffer, tamanio, 0);
	free(buffer);
	log_info(logger, "Aviso al nucleo que termina el programa con pid: %d",
			pid);
}
void solicitarBytes(int numPagina, int offset, int tamanio, uint32_t pid,
		int clienteCPU) {
	/*Primero aceedo a la TLB que posee una lista de paginas, obtengo la pag de lista de paginas,
	 * me fijo que la pagina esta en mp, si no esta en mp,

	 me fijo si puedo asignarle un frame mas, si puedo, si tengo frames libres, traigo la pagina desde SWAP y
	 la cargo al frame, y si no tengo frames libres,aplico algoritmo de reemplazo,

	 si no puedo asignar un frame debido a la cantidad limitada que se le puede dar al PROC,
	 aplico un algoritmo de reemplazo local*/
	void*bytes;
	int nroframe;
	int validacion;
	uint32_t respuestaACPU;
	//chequearPedidoValido(pid,numPagina);
	if (entradasTLB > 0) {
		nroframe = buscarenTLB(pid, numPagina);
		if (nroframe > -1) { //Si entra aca, es un TLB HIT
			bytes = lecturaMemoria((uint32_t) nroframe, offset, tamanio);
			actualizarBitUltimoAccesoTLB(pid, nroframe);
			respuestaACPU=OK_SOLICITUD_UMC;
			send(clienteCPU,&respuestaACPU,sizeof(uint32_t),0);
			send(clienteCPU, bytes, tamanio, 0);
			free(bytes);
			return;
		}
	}

	nroframe = buscarEnListaProcesos(pid, numPagina);
	if (nroframe == OVERFLOW) {
		//Overflow, realizar op pertinentes
		finalizarPrograma(pid);
		avisarNucleoFinProgramaPorOverFlow(pid);
		avisarSwapFinPrograma(pid);
		log_error(logger, "Overflow, finalizar el programa con pid: %d", pid);
		uint32_t error = ERROR_OVERFLOW;
		send(clienteCPU, &error, sizeof(uint32_t), 0);
		return;
	}
	if (nroframe > -1) {
		bytes = lecturaMemoria((uint32_t) nroframe, offset, tamanio);
		if (entradasTLB > 0) {
			cargarenTLB(pid, numPagina, nroframe);
			actualizarBitUltimoAccesoTLB(pid,nroframe);
		}
		log_info(logger,
				"Se encontro la pagina %d correspondiente al pid %d en memoria principal",
				numPagina, pid);
		respuestaACPU=OK_SOLICITUD_UMC;
		send(clienteCPU,&respuestaACPU,sizeof(uint32_t),0);
		send(clienteCPU, bytes, tamanio, 0);
		free(bytes);
		return;
	} else {
		//Pedir a swap que me traiga la pagina
		void*buffer = malloc(tamanioFrame);
		log_info(logger,
				"No se encontro la pagina %d correspondiente al pid %d en memoria principal",
				numPagina, pid);
		log_info(logger, "Pido la pagina al swap");
		pedirAlSwap(numPagina, pid, buffer);
		log_info(logger,"llego esto del swap: %s",(char*)buffer);
		validacion = cargarPaginaenMemoria(pid, numPagina, buffer,&nroframe);
		if (validacion == -1) {
			uint32_t error=0; //hablar con cpu para ver como indicar el FIIIIN por no haber frames locales disponibles para hacer reemplazo

			send(clienteCPU, &error, sizeof(uint32_t), 0);
			pthread_mutex_lock(&mutexSwap);
			avisarSwapFinPrograma(pid);
			pthread_mutex_unlock(&mutexSwap);
			pthread_mutex_lock(&mutexNucleo);
			avisarNucleoFinPrograma(pid);
			pthread_mutex_unlock(&mutexNucleo);
			return;
		} else {
			bytes = lecturaMemoria(nroframe, offset, tamanio);
			respuestaACPU=OK_SOLICITUD_UMC;
			send(clienteCPU,&respuestaACPU,sizeof(uint32_t),0);
			log_info(logger,"Envio estos bytes: %s",(char*)bytes);
			send(clienteCPU, bytes, tamanio, 0);
			return;
		}
	}
}
void almacenarBytesPagina(int numPagina, int offset, int tamanio,
		void*buffer, uint32_t pid, int clienteCPU) {
	int nroframe;
	int validacion;
	uint32_t respuestaACPU;
	//int validacion;
	//Chequear Stack Overflow
	if (entradasTLB > 0) {
		nroframe = buscarenTLB(pid, numPagina);
		if (nroframe > -1) { //TLB HIT
			escrituraMemoria((uint32_t) nroframe, offset, tamanio, buffer);
			actualizarBitModificado(nroframe);
			actualizarBitUltimoAccesoTLB(pid, nroframe);
			respuestaACPU=OK_SOLICITUD_UMC;
			send(clienteCPU,&respuestaACPU,sizeof(uint32_t),0);
			return;
		}
	}
	nroframe = buscarEnListaProcesos(pid, numPagina);
	if (nroframe == OVERFLOW) {
		//Overflow
		finalizarPrograma(pid);
		pthread_mutex_lock(&mutexNucleo);
		avisarNucleoFinProgramaPorOverFlow(pid);
		pthread_mutex_unlock(&mutexNucleo);
		pthread_mutex_lock(&mutexSwap);
		avisarSwapFinPrograma(pid);
		pthread_mutex_unlock(&mutexSwap);
		log_error(logger, "Overflow, finalizar el programa con pid: %d", pid);
		uint32_t error = ERROR_OVERFLOW;
		send(clienteCPU, &error, sizeof(uint32_t), 0);
		return;
	}
	if (nroframe > -1) {
		if (entradasTLB > 0) {
			cargarenTLB(pid, numPagina, nroframe);
			actualizarBitUltimoAccesoTLB(pid,nroframe);
		}
		respuestaACPU=OK_SOLICITUD_UMC;
		send(clienteCPU,&respuestaACPU,sizeof(uint32_t),0);
		escrituraMemoria((uint32_t) nroframe, offset, tamanio, buffer);
		actualizarBitModificado(nroframe);
		return;
	}
	else{
	void*bufferRecibo = malloc(tamanioFrame);
			log_info(logger,
					"No se encontro la pagina %d correspondiente al pid %d en memoria principal",
					numPagina, pid);
			log_info(logger, "Pido la pagina al swap");
			pedirAlSwap(numPagina, pid, bufferRecibo);
			validacion = cargarPaginaenMemoria(pid, numPagina, bufferRecibo,&nroframe);
			if (validacion == -1) {
				int mensaje; //hablar con cpu para ver como indicar el FIIIIN por no haber frames locales disponibles para hacer reemplazo
				send(clienteCPU, &mensaje, sizeof(int), 0);
				pthread_mutex_lock(&mutexSwap);
				nroframe = buscarEnListaProcesos(pid, numPagina);
				if (nroframe == OVERFLOW) {
					//Overflow, realizar op pertinentes
					finalizarPrograma(pid);
				avisarSwapFinPrograma(pid);
				pthread_mutex_unlock(&mutexSwap);
				pthread_mutex_lock(&mutexNucleo);
				avisarNucleoFinPrograma(pid);
				pthread_mutex_unlock(&mutexNucleo);
				return;
			} else {
				respuestaACPU=OK_SOLICITUD_UMC;
							send(clienteCPU,&respuestaACPU,sizeof(uint32_t),0);
				escrituraMemoria(nroframe,offset,tamanio,buffer);
				return;
			}
	}
}
}
void finalizarPrograma(uint32_t pid) {
	int indiceProc = encontrarPosicionEnProcesos(pid);
	//MUTEX
	liberarMarcos(pid);
	pthread_mutex_lock(&mutexProcs);
	liberarPaginas(indiceProc);
	list_remove_and_destroy_element(listaDeProcesos, indiceProc,
			(void*) proc_Destroy);
	pthread_mutex_unlock(&mutexProcs);

	if (entradasTLB > 0) {
		//Limpiar TLB
		limpiarEntradasTLB(pid);
	}
	//MUTEX_SWAP
	//InformarAlSwap
	pthread_mutex_lock(&mutexSwap);
	avisarSwapFinPrograma(pid);
	pthread_mutex_unlock(&mutexSwap);
	log_info(logger, "Se termino el programa con pid: %d", pid);
}

void cambioProceso(uint32_t idNuevoPrograma, uint32_t * idProcesoActivo) {
	if (entradasTLB > 0) {
		//Limpiar TLB
		limpiarEntradasTLB(*idProcesoActivo);
	}
	uint32_t pidViejo=*idProcesoActivo;
	(*idProcesoActivo) = idNuevoPrograma;
	log_info(logger, "Cambio el proceso %d por otro con pid %d",
			pidViejo, idNuevoPrograma);

}
void inicializarListaFrames() {
	int i;
	for (i = 0; i < cantidadDeFrames; i++) {
		t_nodo_lista_frames * nodoAux = malloc(sizeof(t_nodo_lista_frames));
		nodoAux->pid = 0;
		nodoAux->id_frame = (uint32_t) i;
		nodoAux->bitModificado = 0;
		nodoAux->bitReferencia = 0;
		list_add(listaDeFrames, nodoAux);
	}
	log_info(logger, "Frames creados");
}
void inicializarTLB() {
	int j = 0;
	for (j = 0; j < entradasTLB; j++) {
		t_entrada_TLB*nodoTLBAux = malloc(sizeof(t_entrada_TLB));
		nodoTLBAux->ultimo_acceso = accesosAMemoria;
		list_add(tlb, nodoTLBAux);
	}
	log_info(logger, "TLB creada");
}
void inicializarListasYVariables(t_config* cfgUMC) {
	retardoUMC = config_get_int_value(cfgUMC, "RETARDO");
	entradasTLB = config_get_int_value(cfgUMC, "ENTRADAS_TLB");
	tamanioFrame = config_get_int_value(cfgUMC, "TAMANIO_FRAME");
	cantidadDeFrames = config_get_int_value(cfgUMC, "CANT_FRAMES");
	algoritmo=config_get_string_value(cfgUMC,"ALGORITMO");
	marcos_x_proc=config_get_int_value(cfgUMC,"MARCO_X_PROC");
	listaDeProcesos = list_create();
	listaDeFrames = list_create();
	inicializarListaFrames();
	if (entradasTLB > 0) {
		tlb = list_create();
		inicializarTLB();
	}
	tamanioTotalmemoria = (tamanioFrame) * (cantidadDeFrames);
	memoriaprincipal = malloc(tamanioTotalmemoria);
}


