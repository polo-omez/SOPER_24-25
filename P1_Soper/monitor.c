/**
 * @file monitor.c
 * @author Diego Gómez y Pablo Gómez.
 * @brief Implementación de la función monitor.
 * @version 1.0
 * @date 2025-02-18
 */
#include "miner.h"
#include "monitor.h"
#include "pow.h"
#include "mrush.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdatomic.h>

/**
 * @brief Valida si una solución encontrada por un minero es aceptada o no.
 * @param target Valor objetivo que se busca alcanzar con la función hash.
 * @param solution Valor encontrado por el miner, cuyo hash será verificado.
 * @return 1 si la solución es válida (hash(solution) == target), 0 en caso contrario.
 */
long int monitor(long int target, long int solution){
    /* La función calcula el hash del valor solution y lo compara con target. */
    if (pow_hash(solution) == target){
        /* Si coinciden, la solución es aceptada y se imprime un mensaje de éxito. */
        printf("Soluction accepted: %08ld --> %08ld\n", target, solution);
        return 1;
    } else{
        /* En caso contrario, la solución es rechazada. */
        printf("Soluction rejected: %08ld !-> %08ld\n", target, solution);
        return 0;
    }
}


/**
 * @brief Ejecuta las rondas del Monitor.
 *
 * Lee en cada ronda el dato objetivo y la solución a través de la tubería (pipe1),
 * verifica la solución usando la función monitor, y envía la respuesta de vuelta 
 * a través de la tubería (pipe2).
 *
 * @param rounds Número de rondas a ejecutar.
 * @param pipe1_read Descriptor de lectura de pipe1.
 * @param pipe2_write Descriptor de escritura de pipe2.
 */
void ejecutarRondasMonitor(int rounds, int pipe1_read, int pipe2_write) {
    int i = 0;
    long monitor_target, monitor_solution;
    int monitor_response = 1; /* Respuesta del monitor */

    do {
        /* Monitor lee el dato objetivo a través de pipe1 */
        if (read(pipe1_read, &monitor_target, sizeof(long)) == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        /* Monitor lee la solución a través de pipe1 */
        if (read(pipe1_read, &monitor_solution, sizeof(long)) == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        /* Verifica la solución usando la función monitor */
        monitor_response = monitor(monitor_target, monitor_solution);

        /* Envía el resultado de vuelta a través de pipe2. */
        if (write(pipe2_write, &monitor_response, sizeof(int)) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        /* Incremento de la variable bucle*/
        i++;

    } while ((i < rounds) && (monitor_response == 1));
}

