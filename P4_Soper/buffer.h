#ifndef _BUFFER_H
#define _BUFFER_H

#include "miner.h"
#include "shm_miner.h"

#define SIZE 6
#define MSG_MAX 100

/** 
 * Cada desafío resuelto se almacena en un bloque 
 */
typedef struct _block {
    int id; /* Un identificador del bloque, que se incrementa en cada ronda. */
    long target; /* El target, resultado del que se desea hallar el operando. */
    long solution; /* La solución, operando que permite hallar el target. */
    int is_valid_solution; /* 1 si la solución fue validada, 0 en caso contrario. */
    pid_t winner; /* El PID del minero ganador, el que ha resuelto la POW. */
	int totalVotes; /* El número de votos totales que se han usado para evaluar ese bloque.*/
	int posVotes; /* El número de votos positivos  que se han usado para evaluar ese bloque.*/
    int size; /* Número de wallets en este bloque. */
    Wallet wallets[MAX_MINER]; /* Las carteras de los mineros actuales, incluyendo sus PID y las monedas que han obtenido hasta ese momento. */
} block;

/** 
 * Buffer circular de bloques para coordinación con el monitor 
 */
typedef struct _circular_buffer{
    int pwrite;
    int pread;
    int size;
    sem_t sem_empty; 
    sem_t sem_fill; 
    sem_t sem_mutex;
    block bloques[SIZE];
    MiningData *message;
} circular_buffer;

/**
 * Inicializa un buffer circular:
 *
 * @param p_buffer  Puntero al buffer sin inicializar.
 * @return          Mismo puntero para encadenamiento.
 */
circular_buffer *buffer_init(circular_buffer *p_buffer);

/**
 * Destruye un buffer circular:
 *
 * @param p_buffer  Puntero al buffer a destruir.
 */
void buffer_destroy(circular_buffer *p_buffer);

/**
 * Añade un bloque al buffer circular:
 *
 * @param p_buffer  Puntero al buffer.
 * @param bloq      Bloque a insertar.
 */
void buffer_addElem(circular_buffer *p_buffer, block bloq);

/**
 * Extrae un bloque del buffer circular:
 *
 * @param p_buffer  Puntero al buffer.
 * @return          Bloque leído.
 */
block buffer_extractElem(circular_buffer *p_buffer);

#endif