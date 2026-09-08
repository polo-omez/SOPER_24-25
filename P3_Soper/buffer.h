#ifndef _BUFFER_H
#define _BUFFER_H

#include <semaphore.h> 

#define SIZE 6
#define MSG_MAX 100

typedef struct _block {
    long target;
    long solution;
    int is_valid_solution;
} block;

typedef struct _circular_buffer{
    int pwrite;
    int pread;
    int size;
    sem_t sem_empty; 
    sem_t sem_fill; 
    sem_t sem_mutex;
    block bloques[SIZE];
    char msg[MSG_MAX];
} circular_buffer;

circular_buffer *buffer_init(circular_buffer *p_buffer);

void buffer_destroy(circular_buffer *p_buffer);


void buffer_addElem(circular_buffer *p_buffer, block bloq);

block buffer_extractElem(circular_buffer *p_buffer);

#endif