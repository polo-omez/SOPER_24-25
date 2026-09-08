#ifndef HANDLER_H
#define HANDLER_H

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
 * @brief Manejador para la señal SIGTERM.
 *
 * Limpia los recursos (semáforos y ficheros) antes de terminar el programa.
 *
 * @param sig Señal recibida (SIGTERM).
 */
void handler_term(int sig);

/**
 * @brief Manejador para la señal SIGALRM.
 *
 * Indica que se ha cumplido el tiempo máximo de ejecución estableciendo
 * la variable got_alarm a 1 y envía una señal SIGINT al proceso actual.
 *
 * @param sig Señal recibida (SIGALRM).
 */
void handler_alarm(int sig);

/**
 * @brief Manejador para la señal SIGUSR1.
 *
 * Indica que se ha recibido la señal de inicio o reanudación de votación
 * estableciendo la variable got_usr1 a 1.
 *
 * @param sig Señal recibida (SIGUSR1).
 */
void handler_usr1(int sig);

/**
 * @brief Manejador para la señal SIGUSR2.
 *
 * Indica que se ha recibido la señal de votación estableciendo la variable
 * got_usr2 a 1.
 *
 * @param sig Señal recibida (SIGUSR2).
 */
void handler_usr2(int sig);

/**
 * @brief Manejador para la señal SIGINT.
 *
 * Inicia el proceso de finalización del sistema de votación. Envía SIGTERM
 * a todos los procesos Votante y espera a que terminen, liberando también
 * los recursos compartidos. Muestra un mensaje de salida adecuado.
 *
 * @param sig Señal recibida (SIGINT).
 */
void handler_int(int sig);

#endif