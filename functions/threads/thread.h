#pragma once 

typedef struct thread_identifier{
    int identifier;
    int rowsToRead;
    int colsAmount;
    char* filter_filename;
    float** rowsToWork;
}ThreadContext;