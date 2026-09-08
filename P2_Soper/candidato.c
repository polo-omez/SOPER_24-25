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

void procesarCandidato(pid_t votante, int n_proc, struct timespec t_1, struct timespec t_250, sem_t *sem, sem_t *sem2) {
    FILE *file = NULL, *filePID = NULL, *fileVoting = NULL;
    pid_t pid;
    int balance = 0;
    char voto, tmp;
    int countVotes = 0;

    /* Postea al semáforo sem2 para coordinar la sincronización entre procesos */
    if (sem_post(sem2) == -1) {
        perror("sem_post");
    }

    /* Reinicia el archivo de votación: se abre en modo escritura para vaciarlo */
    fileVoting = fopen(VOTING_FILE, "w");
    if (fileVoting == NULL) {
        perror("fopen");
        sem_close(sem);
        sem_close(sem2);
        exit(EXIT_FAILURE);
    }
    fclose(fileVoting);

    /* Abre el archivo que contiene los PID de los procesos */
    filePID = fopen(FILE_NAME, "r");
    if (filePID == NULL) {
        perror("fopen");
        sem_close(sem);
        sem_close(sem2);
        exit(EXIT_FAILURE);
    }

    /* Recorre el archivo leyendo cada PID y envía SIGUSR2 a todos menos al votante actual */
    while (fread(&pid, sizeof(pid), 1, filePID) == 1) {
        if (pid != votante) {
            if (kill(pid, SIGUSR2) == -1) {
                perror("kill");
                fclose(filePID);
                sem_close(sem);
                sem_close(sem2);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (ferror(filePID)) {
        perror("fread");
        fclose(filePID);
        sem_close(sem);
        sem_close(sem2);
        exit(EXIT_FAILURE);
    }
    fclose(filePID);
    filePID = NULL;

    /* Se inicia la recolección de votos de los votantes */
    printf("Candidate %d => [ ", votante);
    while (countVotes < (n_proc - 1)) {
        /* Pausa breve entre cada recolección */
        nanosleep(&t_1, NULL);

        /* Espera a que el semáforo sem2 sea liberado por otro proceso */
        sem_wait(sem2);

        /* Abre el archivo de votación para leer los votos emitidos */
        file = fopen(VOTING_FILE, "r");
        if (file == NULL) {
            perror("fopen");
            sem_close(sem);
            sem_close(sem2);
            exit(EXIT_FAILURE);
        }

        /* Inicializa la semilla para números aleatorios */
        srand(time(NULL));

        /* Lee cada voto del archivo y actualiza el balance */
        while (fread(&tmp, sizeof(char), 1, file) == 1) {
            if (tmp == 'Y') {
                balance += 1;
            } else {
                balance -= 1;
            }
            fwrite(&tmp, sizeof(char), 1, stdout);
            countVotes++;
        }
        fclose(file);

        /* Libera el semáforo sem2 para que otros procesos puedan continuar */
        sem_post(sem2);
    }

    /* El candidato decide su propio voto de forma aleatoria */
    if (rand() % 2){
        voto = 'Y';
        balance += 1;
    }else{
        voto = 'N';
        balance -= 1;
    }

    /* Imprime el resultado final en función del balance obtenido */
    if (balance < 0)
        printf("%c ]=> Rejected\n", voto);
    else
        printf("%c ]=> Accepted\n", voto);

    /* Libera el semáforo principal para que otro proceso pueda actuar como candidato */
    sem_post(sem);

    /* Espera un tiempo adicional antes de notificar a los votantes */
    nanosleep(&t_250, NULL);

    /* Reabre el archivo que contiene los PID para notificar la finalización */
    file = fopen(FILE_NAME, "r");
    if (file == NULL) {
        perror("fopen");
        sem_close(sem);
        sem_close(sem2);
        exit(EXIT_FAILURE);
    } 

    /* Envía la señal SIGUSR1 a todos los procesos (excepto al votante actual)
       leyendo hasta el final del archivo */
    while (fread(&pid, sizeof(pid), 1, file) == 1) {
        if (pid != votante) {
            if (kill(pid, SIGUSR1) == -1) {
                perror("kill");
                fclose(file);
                sem_close(sem);
                sem_close(sem2);
                exit(EXIT_FAILURE);  
            }
        }
    }
    if (ferror(file)) {
        perror("fread");
        fclose(file);
        sem_close(sem);
        sem_close(sem2);
        exit(EXIT_FAILURE);
    }
    fclose(file);
}