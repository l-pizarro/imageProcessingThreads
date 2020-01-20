#include "general.h"

pthread_barrier_t* barriers;
int** matrix_buffer;
int img_to_read = 1;
int minLevel = 0;
int underLevel = 0;
int total = 0;


void notifyTheFiller(int tvalue) {
    matrix_buffer = (int**)calloc(tvalue, sizeof(int*));
    for (int i=0; i<tvalue; i++)
        matrix_buffer[i] = (int*)calloc(tvalue, sizeof(int));

    pthread_t master;

    pthread_create(&master, NULL, fillBuffer, (void *) tvalue);

    pthread_join(master, NULL);
}

void * fillBuffer(void* tvalue) {
    int size = (int) tvalue;

    printf("Tengo que llenar el buffer, con este tamaño: %d\n", size);

    char filename[100];
    sprintf(filename, "testImages/imagen_%d.png", img_to_read);

    FILE * image_readed;
    image_readed = fopen (filename, "r");
    
    if (!image_readed) {
        printf("Error! no abrí archivo");
        abort();
    }

    ImageStorage*  image = createImageStorage(); 
    image->png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (! image->png_ptr) {
        printf("Error! no hice el png ptr");
        abort();
    }
    image->info_ptr = png_create_info_struct (image->png_ptr);
    if (! image->png_ptr) {
        printf("Error! no hice el info ptr");
        abort();
    }
    png_init_io (image->png_ptr, image_readed);
    png_read_png (image->png_ptr, image->info_ptr, 0, 0);
    png_get_IHDR (image->png_ptr, image->info_ptr, & image->width, & image->height, & image->bit_depth,
          & image->color_type, & image->interlace_method, & image->compression_method,
          & image->filter_method);

    image->rows = png_get_rows (image->png_ptr, image->info_ptr);

    // TRANSFORM IMAGE TO MATRIX
    int** image_matrix = imageToInt(image);

    int rows = image->height;
    int cols = png_get_rowbytes (image->png_ptr, image->info_ptr);
    int i = 0;
    int j = 0;

    while (i < size && i < rows){
        j = 0;
        while (j < size && j < cols){
            matrix_buffer[i][j] = image_matrix[i][j];
            j++;
        }
        i++;
    }

    return NULL;
}

void* syncThreads(void* param) {
    ThreadContext* threadContext = (ThreadContext*) param;
    reader(threadContext);
    pthread_barrier_wait(&barriers[0]);
    float** convolvedMatrix = applyConvolution(threadContext);
    printf("Hebra %d: Ya filtró la imagen\n", threadContext->identifier);
    pthread_barrier_wait(&barriers[1]);
    float** rectificated = rectification(threadContext, convolvedMatrix);
    printf("Hebra %d: Ya rectificó la imagen\n", threadContext->identifier);
    pthread_barrier_wait(&barriers[2]);
    int rows;
    int cols;
    float** pooled = pooling(threadContext, rectificated, &rows, &cols);
    printf("Hebra %d: Ya agrupó la imagen [%d][%d]\n", threadContext->identifier, rows, cols);
    pthread_barrier_wait(&barriers[3]);
    int underLevelPixels = countUnderLevel(pooled, rows, cols);
    underLevel += underLevelPixels;
    total += rows*cols;
    printf("Hebra %d: Encontró %d pixeles bajo el umbral, de un total de %d\n", threadContext->identifier, underLevelPixels, rows*cols);
    pthread_barrier_wait(&barriers[4]);
    return NULL;
    //sync all threads
    //all the other stages.
}

void reader(ThreadContext* thread) {
    int z = 0;
    for (int i = thread->identifier * thread->rowsToRead; i < (thread->identifier + 1) * thread->rowsToRead, z < thread->rowsToRead; i++, z++){
        for (int j = 0; j < thread->colsAmount; j++){
            thread->rowsToWork[z][j] = matrix_buffer[i][j];
        }
    }
}

int countUnderLevel(float** pooledMatrix, int rows, int cols) {
    int pixelsUnderLevel = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (pooledMatrix[i][j] < minLevel) {
                pixelsUnderLevel++;
            }
        }
    }
    return pixelsUnderLevel;
}

void init_pipeline(int cvalue, int hvalue, int tvalue, int nvalue, char* mvalue, int bflag) {   
    underLevel = 0;
    total = 0;
    minLevel = nvalue;

    int rowsToRead = tvalue / hvalue;
    if (rowsToRead % 3 != 0){
        printf("La relación entre buffer y hebras debe ser múltiplo de 3 (%d/%d no es múltiplo de 3)", tvalue, hvalue);
        exit(1);
    }


    pthread_t* thread_array = (pthread_t*)calloc(hvalue, sizeof(pthread_t));
    barriers = (pthread_barrier_t*) calloc(5, sizeof(pthread_barrier_t));

    for (int i = 0; i < 5; i++)
        pthread_barrier_init(&barriers[i], NULL, hvalue);

    notifyTheFiller(tvalue);

    //The master thread has filled the buffer. Now the slaves threads must go to work.

    for (int i = 0; i < hvalue; i++){
        ThreadContext* threadContext = (ThreadContext*)calloc(1, sizeof(ThreadContext));
        threadContext->identifier = i;
        threadContext->rowsToRead = rowsToRead;
        threadContext->colsAmount = tvalue;
        threadContext->rowsToWork = (float**)calloc(rowsToRead, sizeof(float*));
        threadContext->filter_filename = (char*)calloc(100, sizeof(float*));
        strcpy(threadContext->filter_filename, mvalue);
        for (int j = 0; j < rowsToRead; j++)
            threadContext->rowsToWork[j] = (float*)calloc(tvalue, sizeof(float));

        pthread_create(&thread_array[i], NULL, syncThreads, (void*) threadContext);
    }

    for (int i = 0; i < hvalue; i++){
        pthread_join(thread_array[i], NULL);
    }

    for (int i = 0; i < tvalue; i++){
        free(matrix_buffer[i]);
    }
    free(matrix_buffer);

    printf("La hebra padre determina que un %f%c de los pixeles está por debajo del umbral\n", (underLevel/(total*1.0))*100, 37);
    if (img_to_read < cvalue){
        img_to_read++;
        init_pipeline(cvalue, hvalue, tvalue, nvalue, mvalue, bflag);
    }

    return;
}

void init_program(int argc, char **argv) {
	int     cvalue = 0;
    int     hvalue = 0;
    int     tvalue = 0;
    int     nvalue = 0;
    int     bflag   = 0;
    char    mvalue[50];

    int command;
    opterr = 0;

    while ((command = getopt(argc, argv, "c:h:t:m:n:b")) != -1) {
        switch (command) {
            case 'c':
                sscanf(optarg, "%d", &cvalue);
                if (cvalue <= 0){
                    perror("Se debe analizar al menos 1 imagen. (El valor de C no puede ser menor a 0)");
                    exit(1);
                }
                break;
            case 'h':
                sscanf(optarg, "%d", &hvalue);
                if (hvalue <= 0){
                    perror("La cantidad de hebras debe ser mayor a 0");
                    exit(1);
                }
                break;
            case 't':
                sscanf(optarg, "%d", &tvalue);
                if (tvalue <= 0 || tvalue % 3 != 0){
                    perror("El valor del buffer no puede ser menor a 0");
                    exit(1);
                }
                break;
            case 'm':
                sscanf(optarg, "%s", mvalue);
                break;
            case 'n':
                sscanf(optarg, "%d", &nvalue);
                if (nvalue < 0 || nvalue > 100){
                    perror("El umbral debe ser un valor entre 0 y 100");
                    exit(1);
                }
                break;
            case 'b':
                bflag = 1;
                break;
            case '?':
                if (optopt == 'c' || optopt == 'm' || optopt == 'n') {
                    fprintf(stderr, "Opcion -%c requiere un argumento.\n", optopt);
                }
                else if (isprint(optopt)) {
                    fprintf(stderr, "Opcion desconocida '-%c'.\n", optopt);
                }
                else {
                    fprintf(stderr, "Opción con caracter desconocido '\\x%x'.\n", optopt);
                }
            default:
                abort();
        }
    }

    if (hvalue > tvalue){
        perror("No pueden haber más hebras que filas en el buffer.");
        exit(1);
    }
	init_pipeline(cvalue, hvalue, tvalue, nvalue, mvalue, bflag);
}