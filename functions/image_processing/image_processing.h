#pragma once
#include <png.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include "../threads/thread.h"

typedef struct imageStorage
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width;
    png_uint_32 height;
    int bit_depth;
    int color_type;
    int interlace_method;
    int compression_method;
    int filter_method;
    png_bytepp rows;
} ImageStorage;

//Entradas: No posee entradas
//Funcionamiento: Es la encargada de solicitar memoria para un puntero a la estructura que albergará una imagen PNG.
//Salidas: ImageStorage* es un puntero a un espacio de memoria destinado a albergar una imagen PNG.
ImageStorage* createImageStorage();

//Entradas: ImageStorage* image -> Corresponde a un puntero a una estructura que contiene una imagen PNG.
//Funcionamiento: Transforma la estructura de imagen PNG a una matriz de números enteros.
//Salidas: int** matriz de punteros que representa a cada pixel de la imagen.
int** imageToInt(ImageStorage* image);

//Entradas: ThreadContext* thread -> Corresponde al "contexto" de una hebra. Contiene su identificador, la cantidad de 
//                            filas que debe procesar, entre otros datos importantes.
//Funcionamiento: Cuando una hebra entra a esta función, se aplica la matriz de convolución a las filas a cargo
//                de esa hebra.
//Salidas: float** matriz de flotantes que representa a la sección de la imagen que cada hebra procesa, tras 
//         aplicar la matriz de convolución.
float** applyConvolution(ThreadContext* thread);

//Entradas: ThreadContext* thread -> Corresponde al "contexto" de una hebra. Contiene su identificador, la cantidad de 
//                            filas que debe procesar, entre otros datos importantes.
//          float** convolvedMatrix -> Matriz de flotantes que corresponde a la porción que una hebra ya filtró.
//Funcionamiento: Dentro de la porción de la imagen, cada hebra rectifica la porción que le corresponde.
//Salidas: float** matriz de flotantes que representa a la sección de la imagen que cada hebra procesa, tras 
//         rectificar.
float** rectification(ThreadContext* thread, float** convolvedMatrix);

//Entradas: float* array -> Arreglo de flotantes en donde se buscará el número mayor.
//          int len -> Corresponde al largo del arreglo.
//Funcionamiento: Se busca el número mayor dentro de un arreglo de flotantes.
//Salidas: float número flotante que es el mayor dentro del arreglo.
float findMax(float* array, int len);

//Entradas: ThreadContext* thread -> Corresponde al "contexto" de una hebra. Contiene su identificador, la cantidad de 
//                            filas que debe procesar, entre otros datos importantes.
//          float** rectificatedMatrix -> Matriz de flotantes que corresponde a la porción que una hebra ya rectificó.
//          int* pooledRows -> Paso por referencia que permite saber fuera de la función, la cantidad de filas
//                             que pasaron por el pooling.
//          int* pooledCols -> Paso por referencia que permite saber fuera de la función, la cantidad de columnas
//                             que pasaron por el pooling.
//Funcionamiento: Dentro de la porción de la imagen, cada hebra rectifica la porción que le corresponde.
//Salidas: float** matriz de flotantes que representa a la sección de la imagen que cada hebra procesa, tras 
//         hacer el 'pooling'.
float** pooling(ThreadContext* thread, float** rectificatedMatrix, int* pooledRows, int* pooledCols);
