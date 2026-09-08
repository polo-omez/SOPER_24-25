#ifndef UTILS_H
#define UTILS_H

#define _POSIX_C_SOURCE  199309L 
#define FILE_NAME "voting_info.txt"
#define VOTING_FILE "voting_file.txt"
#define SEM_NAME "/sem"
#define SEM_NAME_2 "/sem2"

#include "handler.h"
#include "candidato.h"
#include "votante.h"

#include <signal.h>

/**
 * @brief Variable global que indica si se ha recibido la alarma (SIGALRM).
 *
 * Se declara como volatile sig_atomic_t para garantizar operaciones
 * atómicas y seguras en el contexto de manejo de señales.
 */
extern volatile sig_atomic_t got_alarm;

/**
 * @brief Variable global que indica si se ha recibido la señal SIGUSR1.
 *
 * Se declara como volatile sig_atomic_t para garantizar operaciones
 * atómicas y seguras en el contexto de manejo de señales.
 */
extern volatile sig_atomic_t got_usr1;

/**
 * @brief Variable global que indica si se ha recibido la señal SIGUSR2.
 *
 * Se declara como volatile sig_atomic_t para garantizar operaciones
 * atómicas y seguras en el contexto de manejo de señales.
 */
extern volatile sig_atomic_t got_usr2;

/**
 * @brief Variable global que indica si se ha iniciado la fase de terminación del proceso.
 *
 * Se declara como volatile sig_atomic_t para garantizar operaciones
 * atómicas y seguras en el contexto de manejo de señales.
 */
extern volatile sig_atomic_t terminating;

#endif