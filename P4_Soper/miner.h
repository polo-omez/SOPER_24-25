#ifndef _MINER_H
#define _MINER_H

#include "pow.h"
#include "shm_miner.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdatomic.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_THREADS 100

#define MAX_TRY 100
#define REG_FILE "registrator"
#define SHM_MINER "/miner_shm"

/**
 * Estructura que define el subrango de búsqueda para cada hilo de minado.
 */
typedef struct _range{
    long int ini;           /* Valor inicial del rango de búsqueda. */
    long int end;           /* Valor final del rango de búsqueda. */
    long int target_value;  /* Es el valor objetivo que los hilos deben buscar. */
} Range;

/**
 * Función principal del minero (padre tras fork).
 *
 * @param self       PID del propio proceso minero.
 * @param n_threads  Número de hilos a usar en cada ronda.
 * @param n_seconds  Tiempo máximo de vida del minero (segundos).
 * @param fd_pipe    Descriptor de escritura del pipe hacia el Registrador.
 * @return           1 al terminar correctamente.
 */
int mining_process(pid_t self, int n_threads, int n_seconds, int fd_pipe);

/**
 * Función que ejecuta cada hilo de minado.
 *
 * @param range  Puntero a una estructura Range con ini, end y target_value.
 * @return       Devuelve, vía pthread_exit, el valor hallado o NULL.
 */
void  *miningThread(void * range);

/**
 * Rutina para el proceso que ha encontrado primero la solución.
 *
 * @param minerData  Puntero a la estructura compartida MiningData.
 * @param pid_miner  PID del proceso ganador que emite este voto.
 */
void voteWinner(MiningData *minerData, pid_t pid_miner);

/**
 * Rutina para procesos perdedores
 *
 * @param minerData     Puntero a la estructura compartida MiningData.
 * @param sigmask_usr2  Máscara de señales usada en sigsuspend para bloquear SIGUSR2.
 */
void voteLoser (MiningData *minerData, const sigset_t sigmask_usr2);

#endif