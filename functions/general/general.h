#pragma once
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <png.h>
#include <pthread.h>
#include <string.h>
#include "barrier_implementation.h"
#include "../image_processing/image_processing.h"
#include "../threads/thread.h"

//Entradas: int argc -> Corresponde a la cantidad argumentos enviados a través de la entrada estándar
//          char** argv -> Corresponde a un arreglo con los argumentos enviados a través de la entrada estándar
//Funcionamiento: Es la encargada de recibir los datos desde la ejecución del programa, para comenzar el pipeline
//Salidas: No retorna.
void init_program(int argc, char **argv);



//Entradas: int cvalue -> Corresponde a la cantidad de imágenes a analizar en el pipeline
//          int nvalue -> Corresponde al umbral de negrura sobre el cual concluir tras realizar los filtrados.
//          char* mvalue -> Corresponde al nombre del archivo que contiene la matriz para realizar la convolución.
//          int bflag -> Corresponde a una bandera que señaliza si los resultados se deben mostrar por salida estándar.
//Funcionamiento: Es la encargada de comenzar el pipeline, entregando los parámetros necesarios para que la primera
//                etapa sea capaz de leer.
//Salidas: No retorna.
void init_pipeline(int cvalue, int hvalue, int tvalue, int nvalue, char* mvalue, int bflag);


void notifyTheFiller(int tvalue);

void * fillBuffer(void* tvalue);

void* syncThreads(void* param);

void reader(ThreadContext* thread);

int countUnderLevel(float** pooledMatrix, int rows, int cols);