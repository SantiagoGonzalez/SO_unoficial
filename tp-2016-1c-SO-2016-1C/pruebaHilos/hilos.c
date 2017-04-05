#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


int x = 0;

typedef struct structHilo {
	int num;
	char* nombre;
} structHilo;

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

void sumar(void* args) {
	structHilo sh = *(structHilo *) args;
	int n = sh.num;
	char* nom = sh.nombre;

	int var;
	for (var = 0; var < 3500; var++) {
		pthread_mutex_lock(&mutex);
		x++;
		x++;
		x++;
		pthread_mutex_unlock(&mutex);


	}
	printf("%s %d: %d \n", nom, n, x);

//	TODO: lo puse como "flag" para ver si seguia ejecutando
//	FILE* fp;
//	fp=fopen("log.txt","a+t");
//	fputs("el hilo no para\n",fp);
//	fclose(fp);
}

void restar() {
	int var;
	for (var = 0; var < 3500; var++) {
		pthread_mutex_lock(&mutex);
		x--;
		pthread_mutex_unlock(&mutex);
	}
	printf("THREAD 2: %d \n", x);

}

void crearHiloDetachable(void * funcion, void * args) {
	pthread_t hilo;
	pthread_attr_t att;
	pthread_attr_init(&att);
	int h1;
	h1 = pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED);
	pthread_create(&hilo, &att, funcion, args);
	puts("delegado");
}

void main(void) {
	int h1, h2;

	char* nom = "THREAD";
	int n = 1;
	structHilo sh;
	sh.num = n;
	sh.nombre = nom;
//TODO:crear hilo aca en main
//	pthread_t hilo1;
//	pthread_attr_t att;
//	pthread_attr_init(&att);
//	h1=pthread_attr_setdetachstate(&att,PTHREAD_CREATE_DETACHED);
//	pthread_create(&hilo1,NULL,(void *)sumar,&sh);

//	pthread_t hilo2;
//	pthread_attr_t att2;
//
//	pthread_mutex_init(&mutex, NULL);
//
//	pthread_attr_init(&att2);
//
//	h2 = pthread_attr_setdetachstate(&att2, PTHREAD_CREATE_DETACHED);
//
//	pthread_create(&hilo2, NULL, (void *) restar, NULL);

	crearHiloDetachable((void *) sumar, &sh);
	crearHiloDetachable((void *) restar,NULL);

//	pthread_join(hilo1,NULL);
//	pthread_join(hilo2, NULL);
	sleep(1);
	printf("TERMINA MAIN: %d \n", x);

//while(1);

//pthread_join(hilo1,NULL);
//pthread_join(hilo2,NULL);
}
