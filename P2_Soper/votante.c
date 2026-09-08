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

void votante(int n_proc, int n_segs) {
    struct sigaction sigterm, sigusr1, sigusr2;
    sigset_t blockSet, prevSet, waitSet, waitSet2;
    struct timespec t_250, t_1;
    sem_t *sem, *sem2;
    pid_t votante_pid;

    /* Inicialización de tiempos: ambos segundos en 0 */
    t_250.tv_sec = t_1.tv_sec = 0;
    /* Configuración de t_250 en 250*100000 nanosegundos */
    t_250.tv_nsec = 250 * 100000;
    /* Configuración de t_1 en 1*100000 nanosegundos */
    t_1.tv_nsec = 1 * 100000;

    /*Bloquear señales críticas antes de instalar manejadores*/
    sigemptyset(&blockSet);
    sigaddset(&blockSet, SIGUSR1);
    sigaddset(&blockSet, SIGUSR2);
    if (sigprocmask(SIG_BLOCK, &blockSet, &prevSet) < 0) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    /* Configuración de SIGTERM */
    memset(&sigterm, 0, sizeof(sigterm));
    sigemptyset(&sigterm.sa_mask);
    sigterm.sa_handler = handler_term;
    if (sigaction(SIGTERM, &sigterm, NULL) < 0) {
        perror("sigaction SIGTERM");
        exit(EXIT_FAILURE);
    }

    /* Configuración de SIGUSR1 */
    memset(&sigusr1, 0, sizeof(sigusr1));
    sigusr1.sa_handler = handler_usr1;
    if (sigaction(SIGUSR1, &sigusr1, NULL) < 0) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }

    /* Configuración de SIGUSR2 */
    memset(&sigusr2, 0, sizeof(sigusr2));
    sigusr2.sa_handler = handler_usr2;
    if (sigaction(SIGUSR2, &sigusr2, NULL) < 0) {
        perror("sigaction SIGUSR2");
        exit(EXIT_FAILURE);
    }

    /* Apertura del primer semáforo con permisos de lectura/escritura y valor inicial 1 */
    if ((sem = sem_open(SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    /* Apertura del segundo semáforo con permisos de lectura/escritura y valor inicial 1 */
    if ((sem2 = sem_open(SEM_NAME_2, O_CREAT, S_IRUSR | S_IWUSR , 1)) == SEM_FAILED) {
        perror("sem_open");
        sem_close(sem);
        exit(EXIT_FAILURE);
    }

    /* Guarda el PID actual del proceso */
    votante_pid = getpid();

    /* Espera a SIGUSR1 para comenzar */
    got_usr1 = 0;
    sigfillset(&waitSet);
    sigdelset(&waitSet, SIGUSR1);
    sigdelset(&waitSet, SIGTERM);
    while (!got_usr1) {
        sigsuspend(&waitSet);
    }

    /* En este punto, ya se recibió SIGUSR1 */

    /* Restaurar la máscara original si se desea que SIGUSR1 y SIGUSR2 queden sin bloqueo */
    if (sigprocmask(SIG_SETMASK, &prevSet, NULL) < 0) {
        perror("sigprocmask restore");
        exit(EXIT_FAILURE);
    }
    
    while(1){
        /* Intenta adquirir el primer semáforo sin bloqueo */
        if (sem_trywait(sem) == 0){
            /* Si se pudo adquirir, procesa candidato */
            procesarCandidato(votante_pid, n_proc, t_1, t_250, sem, sem2);
        } else {
            /* Si no se pudo adquirir el semáforo, procesa votante */
            procesarVotante(sem, sem2);
        }
    }
}

void procesarVotante(sem_t *sem, sem_t *sem2) {
    FILE *vot_file = NULL;
    char voto;

    /* Espera la señal SIGUSR2 si aún no se ha recibido */
    got_usr2 = 0;
    sigset_t waitSet2;
    sigfillset(&waitSet2);
    sigdelset(&waitSet2, SIGUSR2);
    sigdelset(&waitSet2, SIGTERM);
    while (!got_usr2) {
        sigsuspend(&waitSet2);
    }

    /* Selecciona un voto aleatorio */
    if (rand() % 2) {
        voto = 'Y';
    } else {
        voto = 'N';
    }

    /* Bloquea el acceso exclusivo al archivo de votación */
    if (sem_wait(sem2) == -1){
        perror("sem_wait");
        exit(EXIT_FAILURE);
    }

    /* Abre el archivo para agregar el voto */
    vot_file = fopen(VOTING_FILE, "a");
    if (vot_file == NULL){
        perror("fopen");
        sem_close(sem);
        sem_close(sem2);
        exit(EXIT_FAILURE);
    }

    /* Escribe el voto en el archivo */
    if (fwrite(&voto, sizeof(voto), 1, vot_file) != 1){
        perror("fwrite");
        exit(EXIT_FAILURE);
    }
    fclose(vot_file);

    /* Libera el semáforo para otros procesos */
    if (sem_post(sem2) == -1){
        perror("sem_post");
        exit(EXIT_FAILURE);
    }

    /* Espera la señal SIGUSR1 para iniciar la siguiente ronda */
    got_usr1 = 0;
    sigset_t waitSet1;
    sigfillset(&waitSet1);
    sigdelset(&waitSet1, SIGUSR1);
    sigdelset(&waitSet1, SIGTERM);
    while (!got_usr1) {
        sigsuspend(&waitSet1);
    }
}