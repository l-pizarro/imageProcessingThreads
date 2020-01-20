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

//Entradas: int cvalue -> Cantidad de imagenes a leer
//          int hvalue -> Cantidad de hebras a crear
//          int tvalue -> Tamaño del buffer
//          int nvalue -> Umbral de negrura
//          char* mvalue -> Nombre del archivo que contiene la matriz de convolución.
//          int bflag -> "Bandera" que permite determinar si al procesar la imagen se deben mostrar los resultados.
//Funcionamiento: Es el procedimiento principal. Aquí se crea tanto la hebra "master" para llenar el buffer, 
//                como también a las hebras esclavas que procesarán la imagen. También llama a la función que
//                permite la escritura de la imagen PNG, como también si se deben mostrar las conclusiones.
//Salidas: No retorna.
void init_pipeline(int cvalue, int hvalue, int tvalue, int nvalue, char* mvalue, int bflag);

//Entradas: int tvalue -> Cantidad de filas y columnas a leer. Tamaño del buffer.
//Funcionamiento: Crea una hebra "master" que llena el buffer, a partir de una imagen PNG.
//Salidas: No retorna.
void notifyTheFiller(int tvalue);

//Entradas: void* tvalue -> Tamaño del buffer. Llega como void*, por requerimiento de hebras. Luego es casteado a int.
//Funcionamiento: Se lee la imagen PNG, para posteriormente, a partir de una matriz de enteros, copiar
//                la porción solicitada dentro del buffer.
//Salidas: No retorna.
void * fillBuffer(void* tvalue);

//Entradas: void* param -> Puntero al contexto de la hebra. Llega como void*, por requerimiento de hebras.
//                         Luego es casteado a ThreadContext*
//Funcionamiento: Función que permite sincronizar el funcionamiento de cada hebra.
//Salidas: No retorna.
void* syncThreads(void* param);

//Entradas: ThreadContext* thread -> Corresponde al "contexto" de una hebra. Contiene su identificador,  
//                                   la cantidad de filas que debe procesar, entre otros datos importantes.
//Funcionamiento: Toma, a partir del buffer, las filas que una hebra tiene que procesar.
//Salidas: no retorna.
void reader(ThreadContext* thread);

//Entradas: float** pooledMatrix -> Matriz de flotantes que corresponde a la porción que una hebra ya pasó por el 
//                                  proceso de pooling.
//          int rows -> Cantidad de filas en la matriz.
//          int cols -> Cantidad de columnas en la matriz.
//Funcionamiento: Se determina en cada porción de cada hebra, la cantidad de pixeles negros.
//Salidas: int cantidad de pixeles encontrados.
int countUnderLevel(float** pooledMatrix, int rows, int cols);