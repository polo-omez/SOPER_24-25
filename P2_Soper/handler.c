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


void handler_term(int sig){
    sem_unlink(SEM_NAME);
    sem_unlink(SEM_NAME_2);
    unlink(FILE_NAME);
    unlink(VOTING_FILE);
    exit(EXIT_SUCCESS);
}


void handler_alarm(int sig){
    if (!terminating) {
        got_alarm = 1;
        kill(getpid(), SIGINT);
    }
}


void handler_usr1(int sig){
    got_usr1 = 1;
}


void handler_usr2(int sig){
    got_usr2 = 1;
}


void handler_int(int sig) {
    if (terminating)
        return;
    terminating = 1;

    pid_t pid;
    int st, fd;

    /* Abrir el archivo en modo lectura (creándolo si no existe) */
    fd = open(FILE_NAME, O_RDONLY);
    if (fd == -1) {
        perror("Open");
        exit(EXIT_FAILURE);
    }

    /* Leer del archivo los PIDs de los procesos y enviarles SIGTERM. */
    while (read(fd, &pid, sizeof(pid)) > 0) {
        if (kill(pid, SIGTERM) == -1) {
            perror("kill");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    close(fd);
    unlink(FILE_NAME);

    /* Espera a que terminen todos los procesos hijos */
    while ((pid = wait(&st)) > 0);
    
    if (pid == -1 && errno != ECHILD) {
        perror("wait");
        exit(EXIT_FAILURE);
    }

    if (got_alarm)
        write(1, "\nFinishing by alarm\n", 21);
    else
        write(1, "\nFinishing by signal\n", 22);

    exit(EXIT_SUCCESS);
}

