#ifndef VOTANTE_H
#define VOTANTE_H

#include "utils.h"
#include "handler.h"
#include "candidato.h"
#include "votante.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/types.h>  

/**
 * @brief Función principal del proceso Votante.
 *
 * Crea los manejadores de señales, abre y gestiona los semáforos necesarios, 
 * y compite por convertirse en Candidato. En caso de no conseguirlo, ejerce 
 * como Votante, esperando las señales correspondientes para votar.
 *
 * @param n_proc Número total de procesos votantes.
 * @param n_segs Tiempo máximo (en segundos) de ejecución del sistema.
 */
void votante (int n_proc, int n_segs);

/**
 * @brief Lógica que sigue un proceso Votante cuando no se convierte en Candidato.
 *
 * Espera la señal SIGUSR2 para emitir un voto aleatorio y lo registra en un 
 * fichero compartido, controlando el acceso con un semáforo. Luego, aguarda 
 * la señal SIGUSR1 para la siguiente ronda.
 *
 * @param sem Semáforo que controla la candidatura.
 * @param sem2 Semáforo que controla el acceso al fichero de votos.
 */
void procesarVotante(sem_t *sem, sem_t *sem2);

#endif