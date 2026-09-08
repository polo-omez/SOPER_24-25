/**
 * @file miner.c
 * @author Diego Gómez y Pablo Gómez.
 * @brief Implementación de la función miner.
 * @version 1.0
 * @date 2025-02-18
 */
#include "miner.h"
#include "pow.h"
#include "mrush.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>

/**
 * @brief Variable global de sincronización entre hilos.
 * 
 * Se utiliza una variable atómica flag para detener la ejecución de los hilos
 * una vez que uno de ellos ha encontrado una solución válida.
 */
extern atomic_int flag;

/**
 * @brief Función ejecutada por cada hilo de miner.
 *
 * Cada hilo explora un rango de valores buscando un número cuyo hash coincida
 * con un valor objetivo. Si un hilo encuentra una solución, la comunica
 * mediante pipe_exit y detiene la ejecución de otros hilos estableciendo
 * la variable global flag a 1.
 *
 * @param range Puntero a una estructura Range que define el rango de búsqueda del hilo.
 * @return void* Devuelve el valor encontrado si se encuentra una solución, de lo contrario NULL.
 */
void *miner(void *range){
    Range *r = (Range *)range;
    long int i;

    /* Itera sobre el rango asignado al hilo */
    for (i = r->ini; i < r->end; i++){
        /* Si el valor del hash calculado es igual al valor objetivo */
        if (pow_hash(i) == r->target_value){

            /* Escribe el objetivo en el pipe */
            if (write(r->pipe_exit, &r->target_value, sizeof(r->target_value)) == -1){
                perror("write");
                exit(EXIT_FAILURE);
            }

            /* Escribe la solución en un pipe*/
            if (write(r->pipe_exit, &i, sizeof(i)) == -1){
                perror("write");
                exit(EXIT_FAILURE);
            }

            /* Cambia el valor de la variable flag a 1 para indicar que se ha encontrado una solución */
            atomic_store(&flag, 1);

            /* Finaliza el hilo retornando la solución encontrada */
            pthread_exit((void *)(uintptr_t)i);
        }
    }
    /* Si no se encuentra ninguna solución, finaliza con NULL */
    pthread_exit(NULL);
}


/**
 * @brief Ejecuta las rondas de minería creando hilos y coordinando la comunicación con el Monitor.
 *
 * @param rounds Número de rondas a ejecutar.
 * @param n_threads Número de hilos a utilizar.
 * @param target Valor objetivo inicial a buscar.
 * @param pipe_write Descriptor de escritura de la tubería (pipe1[1]) para enviar datos al Monitor.
 * @param pipe_read Descriptor de lectura de la tubería (pipe2[0]) para recibir la respuesta del Monitor.
 * @return int Estado devuelto por el Monitor (1 si la solución fue aceptada, 0 en caso contrario).
 */
int ejecutarRondasMiner(int rounds, int n_threads, long target, int pipe_write, int pipe_read) {
    int i = 0, j, state = 1;
    void *res = NULL, *thread_result = NULL;
    Range range[MAX_THREADS];
    pthread_t threads[MAX_THREADS];
    long range_per_thread = (POW_LIMIT - 1) / n_threads;

    do {
        /* Reinicia la variable global de sincronización para la nueva ronda */
        atomic_store(&flag, 0);

        /* Asigna a cada hilo su subrango de búsqueda y crea el hilo */
        j = 0;
        do {
			/* Inicio del subrango. */
            range[j].ini = j * range_per_thread;

			/* Fin del subrango */
            if (j == (n_threads - 1)) {
                range[j].end = POW_LIMIT - 1;
            } else {
                range[j].end = range_per_thread * (j + 1) - 1;
            }

			/* Asigna el valor objetivo que los hilos deben buscar. */
			if (res != NULL){
				range[j].target_value = (intptr_t)res; 
			} else {
				range[j].target_value = target;
			}

            /* Se asigna el descriptor de la tubería para enviar la solución al Monitor */
            range[j].pipe_exit = pipe_write;

			/* Cada uno ejecuta la función miner */
            if (pthread_create(&threads[j], NULL, miner, &range[j]) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
			
			/* Incremento */
            j++;

        } while ((j < n_threads) && (atomic_load(&flag) == 0));

        /* Espera a que terminen todos los hilos y recoge la posible solución */
        for (j = 0; j < n_threads; j++) {
            if (pthread_join(threads[j], &thread_result) != 0) {
                perror("pthread_join");
                exit(EXIT_FAILURE);
            }
            if (thread_result) {
                res = thread_result;
            }
        }

        /* Lee el estado devuelto por el Monitor a través de la tubería */
        if (read(pipe_read, &state, sizeof(int)) == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        
        /* Incremento */
        i++;

    } while ((i < rounds) && (state == 1));

    return state;
}