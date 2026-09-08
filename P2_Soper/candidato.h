#ifndef CANDIDATO_H
#define CANDIDATO_H

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
 * @brief Lógica principal de un proceso que se convierte en Candidato.
 *
 * El proceso Candidato reinicia el fichero de votación, avisa a los demás
 * procesos para que emitan sus votos, controla la recolección de votos
 * (protegiendo el acceso al fichero con semáforos) y, una vez finalizada
 * la votación, decide si la candidatura es aceptada o rechazada según
 * la mayoría de votos. Finalmente, notifica a los votantes el inicio de
 * una nueva ronda.
 *
 * @param votante PID del proceso que se convierte en Candidato.
 * @param n_proc  Número total de procesos votantes.
 * @param t_1     Intervalo de tiempo (en nanosegundos) para la espera no activa 
 *                durante la comprobación de votos.
 * @param t_250   Intervalo de tiempo (en nanosegundos) para la espera no activa 
 *                antes de iniciar la siguiente ronda.
 * @param sem     Semáforo que controla el acceso a la candidatura.
 * @param sem2    Semáforo que controla el acceso al fichero de votación.
 */
void procesarCandidato(pid_t votante, int n_proc, struct timespec t_1, struct timespec t_250, sem_t *sem, sem_t *sem2);

#endif