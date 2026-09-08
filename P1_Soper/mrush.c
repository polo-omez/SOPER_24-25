/**
 * @file mrush.c
 * @author Diego Gómez y Pablo Gómez.
 * @brief Implementación del sistema de minería con múltiples hilos y procesos.
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
 * @brief Variable global de sincronización entre hilos.
 * 
 * Se utiliza una variable atómica flag para detener la ejecución de los hilos
 * una vez que uno de ellos ha encontrado una solución válida.
 */
atomic_int flag;


/**
 * @brief Función principal del programa.
 * @param argc Número de argumentos pasados al programa.
 * @param argv Arreglo de argumentos (TARGET_INI, ROUNDS, N_THREADS).
 * @return int Código de salida (0 si se ejecuta correctamente, 1 en caso de error).
 */
int main(int argc, char **argv){
	pid_t pid, pid2;
	long int target, rounds, n_threads;
	int pipe1[2], pipe2[2], state;

	/* Comprobación de argumentos de entrada */
	if (argc != 4){
		printf("./mrush <TARGET_INI> <ROUNDS> <N_THREADS>\n");
		return 0;
	}

	target = atoi(argv[1]);
	if (target < 0 || target >= POW_LIMIT){
		printf("The chosen target is out of search limits ([0,%d])\n", POW_LIMIT - 1);
		return 1;
	}

	rounds = atoi(argv[2]);
	if (rounds <= 0){
		printf("The number of rounds must be a positive integer greater than 0\n");
		return 1;
	}

	n_threads = atoi(argv[3]);
	if (n_threads <= 0 || n_threads > MAX_THREADS){
		printf("The number of threads must be between 1 and %d.\n", MAX_THREADS);
		return 1;
	}

	/* El primer proceso, Principal, creará un nuevo proceso usando fork: el proceso Minero.*/
	pid = fork();

	/* Si el retorno es negativo, significa que la creación del proceso ha fallado. */
	if (pid < 0){
		perror("fork");
		exit(EXIT_FAILURE);
	} else if (pid == 0){ /* Proceso hijo (Minero) */

		/* Tuberia 1*/
		if (pipe(pipe1) == -1)
		{
			perror("pipe");
			exit(EXIT_FAILURE);
		}

		/* Tuberia 2*/
		if (pipe(pipe2) == -1)
		{
			perror("pipe");
			exit(EXIT_FAILURE);
		}

		/* Creación del proceso Monitor dentro del proceso Minero */
		pid2 = fork();

		/* Si el retorno es negativo, significa que la creación del proceso ha fallado. */
		if (pid2 < 0){
			perror("fork");
			exit(EXIT_FAILURE);

		} else if (pid2 == 0){ /* Proceso hijo de Minero (Monitor) */

			/* Cierra el extremo de escritura en pipe1 (Minero -> Monitor) */
			close(pipe1[1]);

			/* Cierra el extremo de lectura en pipe2 (Monitor -> Minero) */
			close(pipe2[0]);

			/* Ejecuta las rondas del Monitor */
			ejecutarRondasMonitor(rounds, pipe1[0], pipe2[1]);

			/* Cierra los extremos de las tuberías que ya no se van a usar. */
			close(pipe1[0]);
			close(pipe2[1]);

			exit(EXIT_SUCCESS);
			
		} else{ /* Proceso Minero */

			/* Cierra el extremo de lectura en pipe1 (Minero -> Monitor) */
			close(pipe1[0]);

			/* Cierra el extremo de escritura en pipe2 (Monitor -> Minero) */
			close(pipe2[1]);

			/* Ejecuta las rondas del Minero */
			state = ejecutarRondasMiner(rounds, n_threads, target, pipe1[1], pipe2[0]);

			/*Cierra los extremos de las tuberías que ya no se van a usar.*/
			close(pipe1[1]);
			close(pipe2[0]);

			/*Si state es 0, significa que la solución fue invalidada por el Monitor.*/
			if (!state){
				printf("The solution has been invalidated\n");
				wait(&state);
				if (WIFEXITED(state)){
					printf("Monitor exited with status %d\n", WEXITSTATUS(state));
				} else {
					printf("Monitor exited unexpectedly\n");
				}
				exit(EXIT_FAILURE);
			}

			/* Espera a que el proceso Monitor termine. */
			wait(&state);

			/* Verifica si el Monitor terminó correctamente y muestra su estado de salida. */
			if (WIFEXITED(state)){
				printf("Monitor exited with status %d\n", WEXITSTATUS(state));
			} else {
				printf("Monitor exited unexpectedly\n");
			}
			exit(EXIT_SUCCESS);
		}
	} else { /* Proceso Padre (Principal) */

		/* El programa principal espera a que Miner termine*/
		wait(&state);

		/* Verifica si el Miner terminó correctamente y muestra su estado de salida. */
		if (WIFEXITED(state)) {
			printf("Miner exited with status %d\n", WEXITSTATUS(state));
		} else {
			printf("Miner exited unexpectedly\n");
		}
	}
	exit(EXIT_SUCCESS);
}