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

ImageStorage* createImageStorage();

int** imageToInt(ImageStorage* image);

float** applyConvolution(ThreadContext* thread);

float** rectification(ThreadContext* thread, float** convolvedMatrix);

float findMax(float* array, int len);

float** pooling(ThreadContext* thread, float** rectificatedMatrix, int* pooledRows, int* pooledCols);
