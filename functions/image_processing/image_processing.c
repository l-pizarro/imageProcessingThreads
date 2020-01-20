#include "image_processing.h"

//Entradas: No posee entradas
//Funcionamiento: Es la encargada de solicitar memoria para un puntero a la estructura que albergará una imagen PNG.
//Salidas: ImageStorage* es un puntero a un espacio de memoria destinado a albergar una imagen PNG.
ImageStorage* createImageStorage() {
    return (ImageStorage*)calloc(1, sizeof(ImageStorage));
}


//Entradas: ImageStorage* image -> Corresponde a un puntero a una estructura que contiene una imagen PNG.
//Funcionamiento: Transforma la estructura de imagen PNG a una matriz de números enteros.
//Salidas: int** matriz de punteros que representa a cada pixel de la imagen.
int** imageToInt(ImageStorage* image) {
    png_bytepp  rows            = png_get_rows (image->png_ptr, image->info_ptr);
    int         rowbytes        = png_get_rowbytes (image->png_ptr, image->info_ptr);
    int**       image_matrix    = (int**)calloc(image->height, sizeof(int*));

    for (int i = 0; i < image->height; i++)
    {
        image_matrix[i] = (int*)calloc(rowbytes, sizeof(int));
    }

    for (int i = 0; i < image->height; i++)
    {
        png_bytep row;
        row = rows[i];
        for (int j = 0; j < rowbytes; j++) {
            png_byte pixel;
            pixel = row[j];
            image_matrix[i][j] = pixel;
        }
    }
    
    return image_matrix;
}

//Entradas: ThreadContext* thread -> Corresponde al "contexto" de una hebra. Contiene su identificador, la cantidad de 
//                            filas que debe procesar, entre otros datos importantes.
//Funcionamiento: Cuando una hebra entra a esta función, se aplica la matriz de convolución a las filas a cargo
//                de esa hebra.
//Salidas: float** matriz de flotantes que representa a la sección de la imagen que cada hebra procesa, tras 
//         aplicar la matriz de convolución.
float** applyConvolution(ThreadContext* thread) {   
    int** conv_matrix = (int**)calloc(3, sizeof(int*));

    for (int i=0; i<3; i++)
    {
        conv_matrix[i] = (int*)calloc(3, sizeof(int));
    }

    FILE* file_matrix = fopen(thread->filter_filename, "r");

    if (! file_matrix){
        perror("Error opening file. Quitting...");
    }

    int row = 0;

    while (! feof(file_matrix))
    {
        int a, b, c;
        fscanf(file_matrix, "%d %d %d", &a, &b, &c);
        conv_matrix[row][0] = a;
        conv_matrix[row][1] = b;
        conv_matrix[row][2] = c;
        row++;
    }

    fclose(file_matrix);

    float** filtered_matrix;
    filtered_matrix = (float**)calloc(thread->rowsToRead , sizeof(float*));


    for (int i = 0; i < thread->rowsToRead; i++)
    {
        filtered_matrix[i] = (float*)calloc(thread->colsAmount, sizeof(float));
    }

    for (int i = 1; i < thread->rowsToRead - 1; i++)
    {
        for (int j = 1; j < thread->colsAmount - 1; j++)
        {
            float value = 0;
            value = (thread->rowsToWork[i-1][j-1] * conv_matrix[0][0] + thread->rowsToWork[i-1][j] * conv_matrix[0][1] + thread->rowsToWork[i-1][j+1] * conv_matrix[0][2] + thread->rowsToWork[i][j-1] * conv_matrix[1][0] + thread->rowsToWork[i][j] * conv_matrix[1][1] + thread->rowsToWork[i][j+1] * conv_matrix[1][2] + thread->rowsToWork[i+1][j-1] * conv_matrix[2][0] + thread->rowsToWork[i+1][j] * conv_matrix[2][1] + thread->rowsToWork[i+1][j+1] * conv_matrix[2][2]) / 9;
            filtered_matrix[i][j] = value;
        }
    }

    return filtered_matrix;
}

//Entradas: ThreadContext* thread -> Corresponde al "contexto" de una hebra. Contiene su identificador, la cantidad de 
//                            filas que debe procesar, entre otros datos importantes.
//          float** convolvedMatrix -> Matriz de flotantes que corresponde a la porción que una hebra ya filtró.
//Funcionamiento: Dentro de la porción de la imagen, cada hebra rectifica la porción que le corresponde.
//Salidas: float** matriz de flotantes que representa a la sección de la imagen que cada hebra procesa, tras 
//         rectificar.
float** rectification(ThreadContext* thread, float** convolvedMatrix) {
    for (int i = 0; i < thread->rowsToRead; i++){
        for (int j = 0; j < thread->colsAmount; j++){
            if (convolvedMatrix[i][j] < 0){
                convolvedMatrix[i][j] = 0;
            }
        }
    }

    return convolvedMatrix;
}

//Entradas: float* array -> Arreglo de flotantes en donde se buscará el número mayor.
//          int len -> Corresponde al largo del arreglo.
//Funcionamiento: Se busca el número mayor dentro de un arreglo de flotantes.
//Salidas: float número flotante que es el mayor dentro del arreglo.
float findMax(float* array, int len) {
    float max = array[0];
    for (int i = 0; i < len; i++) {
        if (array[i] > max) {
            max = array[i];
        }
    }
    return max;
}

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
float** pooling(ThreadContext* thread, float** rectificatedMatrix, int* pooledRows, int* pooledCols) {
    int rows = thread->rowsToRead/3;
    int cols = thread->colsAmount/3;

    if (thread->colsAmount%3 != 0) {
        cols = (thread->colsAmount/3) + 1;
    }

    *pooledCols = cols;
    *pooledRows = rows;

    float** pooledSubmatrix = (float**)calloc(rows, sizeof(float*));
    
    for (int i = 0; i < cols; i++) {
        pooledSubmatrix[i] = (float*)calloc(cols, sizeof(float));
    }

    // Find the max value for every 3by3 matrix in rectificatedMatrix
    // This solution can be improved, but for now, this is the fast code: Put all values on a array and find it 
    for (int rowIter = 0; rowIter < rows; rowIter++) {
        for (int colIter = 0; colIter < cols; colIter++) {
            float* nineValues = (float*)calloc(9, sizeof(float));
            nineValues[0] = rectificatedMatrix[(rowIter*3) + 0][(colIter*3 + 0) % thread->colsAmount];
            nineValues[1] = rectificatedMatrix[(rowIter*3) + 0][(colIter*3 + 1) % thread->colsAmount];
            nineValues[2] = rectificatedMatrix[(rowIter*3) + 0][(colIter*3 + 2) % thread->colsAmount];
            nineValues[3] = rectificatedMatrix[(rowIter*3) + 1][(colIter*3 + 0) % thread->colsAmount];
            nineValues[4] = rectificatedMatrix[(rowIter*3) + 1][(colIter*3 + 1) % thread->colsAmount];
            nineValues[5] = rectificatedMatrix[(rowIter*3) + 1][(colIter*3 + 2) % thread->colsAmount];
            nineValues[6] = rectificatedMatrix[(rowIter*3) + 2][(colIter*3 + 0) % thread->colsAmount];
            nineValues[7] = rectificatedMatrix[(rowIter*3) + 2][(colIter*3 + 1) % thread->colsAmount];
            nineValues[8] = rectificatedMatrix[(rowIter*3) + 2][(colIter*3 + 2) % thread->colsAmount];
            pooledSubmatrix[rowIter][colIter] = findMax(nineValues, 9);
        }
    }

    return pooledSubmatrix;
}
