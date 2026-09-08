#include "buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>


circular_buffer *buffer_init(circular_buffer *p_buffer) {
    p_buffer->pwrite = 0;
    p_buffer->pread = 0;
    p_buffer->size = SIZE;

    if (sem_init(&p_buffer -> sem_fill, 1, 0)){
        perror("sem_init");
        free(p_buffer);
        return NULL;
    }

    if (sem_init(&p_buffer -> sem_empty, 1, 5)){
        perror("sem_init");
        sem_close(&p_buffer -> sem_fill);
        free(p_buffer);
        return NULL;
    }

    if (sem_init(&p_buffer -> sem_mutex, 1, 1)){
        perror("sem_init");
        sem_close(&p_buffer -> sem_fill);
        sem_close(&p_buffer -> sem_empty);
        free(p_buffer);
        return NULL; 
    }

    return p_buffer;
}

void buffer_destroy(circular_buffer *p_buffer) {
    if (p_buffer == NULL) 
        return;

    sem_destroy(&p_buffer -> sem_fill);
    sem_destroy(&p_buffer -> sem_empty);
    sem_destroy(&p_buffer -> sem_mutex);
}


void buffer_addElem(circular_buffer *p_buffer, block bloq) {
    p_buffer -> bloques[p_buffer -> pwrite] = bloq;
    p_buffer -> pwrite ++;
    p_buffer -> pwrite %= p_buffer -> size;
}

block buffer_extractElem(circular_buffer *p_buffer) {
    block elem;

    elem = p_buffer -> bloques[p_buffer -> pread];
    p_buffer -> pread ++;
    p_buffer -> pread %= p_buffer->size;

    return elem;
}