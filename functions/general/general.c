#include "general.h"

pthread_barrier_t* barriers;
pthread_mutex_t mutex;
int** matrix_buffer;
float** finalMatrix;
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

    // printf("Tengo que llenar el buffer, con este tamaño: %d\n", size);

    char filename[200];
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

void fillFinalMatrix(ThreadContext* thread, float** pooled, int rows, int cols){
    int z = 0;
    for (int i = thread->identifier * rows; i < (thread->identifier + 1) * rows; i++, z++){
        for (int j = 0; j < cols; j++){
            finalMatrix[i][j] = pooled[z][j];
        }
    }
}

void* syncThreads(void* param) {
    ThreadContext* threadContext = (ThreadContext*) param;
    reader(threadContext);
    pthread_barrier_wait(&barriers[0]);
    float** convolvedMatrix = applyConvolution(threadContext);
    // printf("Hebra %d: Ya filtró la imagen\n", threadContext->identifier);
    pthread_barrier_wait(&barriers[1]);
    float** rectificated = rectification(threadContext, convolvedMatrix);
    // printf("Hebra %d: Ya rectificó la imagen\n", threadContext->identifier);
    pthread_barrier_wait(&barriers[2]);
    int rows;
    int cols;
    float** pooled = pooling(threadContext, rectificated, &rows, &cols);
    // printf("Hebra %d: Ya agrupó la imagen [%d][%d]\n", threadContext->identifier, rows, cols);
    pthread_barrier_wait(&barriers[3]);
    pthread_mutex_lock(&mutex);
    int underLevelPixels = countUnderLevel(pooled, rows, cols);
    underLevel += underLevelPixels;
    total += rows*cols;
    pthread_mutex_unlock(&mutex);
    fillFinalMatrix(threadContext, pooled, rows, cols);
    // printf("Hebra %d: Encontró %d pixeles bajo el umbral, de un total de %d\n", threadContext->identifier, underLevelPixels, rows*cols);
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

void save_png_to_file (float** bitmap, int width, int height)
{
    FILE * fp;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    size_t x, y;
    png_byte ** row_pointers = NULL;
    /* "status" contains the return value of this function. At first
       it is set to a value which means 'failure'. When the routine
       has finished its work, it is set to a value which means
       'success'. */
    int status = -1;
    /* The following number is set by trial and error only. I cannot
       see where it it is documented in the libpng manual.
    */
    int pixel_size = 3;
    int depth = 8;

    char* filename;
    filename = (char*)calloc(200, sizeof(char));
    sprintf(filename, "output/out_%d.png", img_to_read);
    
    fp = fopen (filename, "wb");
    if (! fp) {
        perror("fallo apertura archivo");
        exit(1);
    }

    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        perror("fallé creando una struct linea 95");
        exit(1);
    }
    
    info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) {
        perror("fallé creando una struct linea 101");
        exit(1);
    }
    
    /* Set up error handling. */

    if (setjmp (png_jmpbuf (png_ptr))) {
        perror("error en esta cosa");
        exit(1);
    }
    
    /* Set image attributes. */

    png_set_IHDR (png_ptr,
                  info_ptr,
                  width,
                  height,
                  depth,
                  PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);
    
    /* Initialize rows of PNG. */

    row_pointers = png_malloc (png_ptr, height * sizeof (png_byte *));
    for (y = 0; y < height; y++) {
        png_byte *row = 
            png_malloc (png_ptr, sizeof (uint8_t) * width * pixel_size);
        row_pointers[y] = row;
        for (x = 0; x < width; x++) {
            *row++ = bitmap[x][y];
        }
    }
    
    /* Write the image data to "fp". */

    png_init_io (png_ptr, fp);
    png_set_rows (png_ptr, info_ptr, row_pointers);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    /* The routine has successfully written the file, so we set
       "status" to a value which indicates success. */

    status = 0;
    
    for (y = 0; y < height; y++) {
        // png_free (png_ptr, row_pointers[y]);
    }
    // png_free (png_ptr, row_pointers);
    
    fclose (fp);
}

char* classification(float percent, int threshold){
    return (percent * 1.0 < threshold) ? "YES" : "NO";
}

void showResults(int threshold, float percent){
    if (img_to_read == 1){
        printf("| NOMBRE IMAGEN | PORCENTAJE NEGRURA | NEARLY BLACK (Umbral en %d) |\n", threshold);
    }
    printf("|   imagen_%d    |     %f%c      |             %s              |\n", img_to_read, percent, 37, classification(percent, threshold));
}

void init_pipeline(int cvalue, int hvalue, int tvalue, int nvalue, char* mvalue, int bflag) {  
    img_to_read = cvalue; 
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

    finalMatrix = (float**)calloc(tvalue, sizeof(float*));
    for (int i = 0; i < tvalue; i++){
       finalMatrix[i] = (float*)calloc(tvalue/3, sizeof(float));
    }

    notifyTheFiller(tvalue);

    //The master thread has filled the buffer. Now the slaves threads must go to work.

    for (int i = 0; i < hvalue; i++){
        ThreadContext* threadContext = (ThreadContext*)calloc(1, sizeof(ThreadContext));
        pthread_mutex_init(&mutex, NULL);
        threadContext->identifier = i;
        threadContext->rowsToRead = rowsToRead;
        threadContext->colsAmount = tvalue;
        threadContext->rowsToWork = (float**)calloc(rowsToRead, sizeof(float*));
        threadContext->filter_filename = (char*)calloc(200, sizeof(float*));
        strcpy(threadContext->filter_filename, mvalue);
        for (int j = 0; j < rowsToRead; j++)
            threadContext->rowsToWork[j] = (float*)calloc(tvalue, sizeof(float));

        pthread_create(&thread_array[i], NULL, syncThreads, (void*) threadContext);
    }

    for (int i = 0; i < hvalue; i++){
        pthread_join(thread_array[i], NULL);
    }

    save_png_to_file(finalMatrix, tvalue, tvalue);

    if (bflag == 1)
        showResults(nvalue, (underLevel/(total*1.0))*100);

    for (int i = 0; i < tvalue; i++){
        // free(matrix_buffer[i]);
    }
    // free(matrix_buffer);

    // printf("La hebra padre determina que un %f%c de los pixeles está por debajo del umbral\n", (underLevel/(total*1.0))*100, 37);
    // if (img_to_read < cvalue){
    //     img_to_read++;
    //     init_pipeline(cvalue, hvalue, tvalue, nvalue, mvalue, bflag);
    // }

    return;
}

void init_program(int argc, char **argv) {
	int     cvalue = 0;
    int     hvalue = 0;
    int     tvalue = 0;
    int     nvalue = 0;
    int     bflag   = 0;
    char    mvalue[100];

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
    for (int i = 0; i < cvalue; i++){
	   init_pipeline(i+1, hvalue, tvalue, nvalue, mvalue, bflag);
    }
}