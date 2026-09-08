/**
 * @file monitor.c
 * @author Diego Gómez y Pablo Gómez.
 * @brief Implementación de la función monitor.
 * @version 1.0
 * @date 2025-02-18
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <semaphore.h>

#include "monitor.h"
#include "pow.h"
#include "buffer.h"

/**
 * @brief Valida si una solución encontrada por un minero es aceptada o no.
 * @param target Valor objetivo que se busca alcanzar con la función hash.
 * @param solution Valor encontrado por el miner, cuyo hash será verificado.
 * @return 1 si la solución es válida (hash(solution) == target), 0 en caso contrario.
 */
int monitor(long int target, long int solution) {
    if (pow_hash(solution) == target) {
        return 1;
    }
    else {
        return 0;
    }
}

int main(int argc, char **argv) {
    int i=0, fd_shm;
    circular_buffer *buffer = NULL;
    block block;
    pid_t pid;
    sem_t *sem;
    mqd_t mq;
    MiningData m;

    /* Este proceso arranca a través de un programa que se debe ejecutar sin parámetros: 
        ./ monitor donde monitor es el nombre del ejecutable.*/
    if (argc != 1){
        printf("Formato: %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* El proceso principal, Comprobador, crea e inicializa un segmento de memoria compartida 
    de monitoreo, dándole tamaño, enlazándolo a su espacio de memoria, y creando los semáforos 
    oportunos.*/

    fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd_shm == -1) {
        if (errno == EEXIST) {
            fd_shm = shm_open(SHM_NAME, O_RDWR, 0);
            if (fd_shm == -1) {
                perror("Error opening the shared memory segment");
                exit(EXIT_FAILURE);
            }
            else {
                perror("Shared memory segment open\n");
            }
        }
        else {
            perror("Error creating the shared memory segment\n");
            exit(EXIT_FAILURE);
        }
    }
    else {
        perror("Shared memory segment created\n");
    }


    /*truncate*/ /*Espacio que quieremos usar en la memoria*/
    if (ftruncate(fd_shm, sizeof(circular_buffer)) == -1){
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    /* Mapping of the memory segment. */
    buffer = mmap(NULL, sizeof(circular_buffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (buffer == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    /* Inicializar buffer */
    buffer = buffer_init(buffer);
    if (buffer == NULL){
        exit(EXIT_FAILURE);
    }


    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    block.solution = 0;
    block.wallets[0].pid = -1;


	/* El proceso Comprobador se encarga de recibir y validar los bloques enviados por el 
        proceso Ganador en cada ronda.*/
	if (!pid){ 
        /* Apertura/creación del semáforo */
        sem = sem_open(SEM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
        if (sem == SEM_FAILED) {
            if (errno == EEXIST) {
                sem = sem_open(SEM_NAME, O_RDWR, S_IRUSR | S_IWUSR, 0);
                if (sem == SEM_FAILED) {
                    perror("sem_open");
                    exit(EXIT_FAILURE);
                }
            } else {
                perror("sem_open");
                exit(EXIT_FAILURE);
            }
        }
        sem_post(sem);
        
        /* Apertura/creación de la cola de mensajes */
        mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY | O_EXCL, S_IRUSR | S_IWUSR, &attributes);
        if (mq == (mqd_t)-1) {
            if (errno == EEXIST) {
                mq = mq_open(MQ_NAME, O_RDONLY, S_IRUSR | S_IWUSR, &attributes);
                if (mq == (mqd_t)-1) {
                    perror("mq_open");
                    exit(EXIT_FAILURE);
                }
            } else {
                perror("mq_open");
                exit(EXIT_FAILURE);
            }
        }

        i = 0;
        while(block.solution != -1 && block.wallets[0].pid != 0){
            /* Recibe un bloque a través de la cola de mensajes. */
            if (mq_receive(mq, (char *)&m, sizeof(MiningData), NULL) == -1) {
                perror("mq_recieve");
                block.solution = -1; /* Indicar finalización en el bloque */

                /* Insertar el bloque terminador en el buffer compartido */
                sem_wait(&buffer->sem_mutex);
                buffer_addElem(buffer, block);
                sem_post(&buffer->sem_mutex);

                sem_wait(sem);
                sem_close(sem);
                exit(EXIT_FAILURE);
            }

            /* Comprueba el bloque de nuevo (con independencia de sus votos) ajustando 
                la bandera que indica si es correcto o no.*/

            /* Actualización de los campos del bloque con los datos recibidos */
            block.id = i;
            block.target = m.target;
            block.solution =  m.solution;
            block.winner = m.winner;
            block.posVotes = m.posVotes;
            block.totalVotes = m.totalVotes;
            block.size = m.size;
            memcpy(block.wallets, m.wallets, m.size * sizeof(Wallet));
            
            /* Verificación y validación de la solución */
            if (block.posVotes >= block.totalVotes / 2){
                block.is_valid_solution = monitor(block.target, block.solution);
            }

            /* Una vez comprobado, lo introduce en memoria compartida para que lo lea Monitor, y 
                vuelve a repetir el proceso de recepción/comprobación/escritura.*/
            sem_wait(&buffer->sem_empty);
            sem_wait(&buffer->sem_mutex);
            buffer_addElem(buffer, block);
            sem_post(&buffer->sem_mutex);
            sem_post(&buffer->sem_fill);

            i++;
        }

        /* El proceso Comprobador finaliza cuando recibe un bloque de finalización a través
            de la cola de mensajes, o cuando le llega la señal SIGINT. En cualquiera de los dos casos, notifica
            el cierre del sistema escribiendo un bloque de finalización en memoria compartida para Monitor.
            Por último, espera la finalización del proceso Monitor, imprime un mensaje de aviso en el caso
            de que no termine con el código de salida EXIT_SUCCESS, y termina liberando todos sus recursos.*/
        sem_wait(sem);
        sem_close(sem);
        mq_close(mq);

        mq_unlink(MQ_NAME);
        shm_unlink(SHM_NAME);

        buffer_destroy(buffer);
        munmap(buffer, sizeof(circular_buffer));

        wait(NULL);
        exit(EXIT_SUCCESS);

	} else {  /* El proceso Monitor arranca como hijo del proceso Comprobador. */

        /* El proceso Monitor se encarga de mostrar los bloques por pantalla*/
        while (1) {
            i = 0;
            /* 1. Extrae un bloque, usando el esquema de productor–consumidor. */
            sem_wait(&buffer->sem_fill);
            sem_wait(&buffer->sem_mutex);
            block = buffer_extractElem(buffer);
            sem_post(&buffer->sem_mutex);
            sem_post(&buffer->sem_empty);

            /* Si se recibe el bloque terminador, se sale inmediatamente */
            if (block.solution == -1 || block.wallets[0].pid == 0)
                break;

            /* 2. Lo muestra por pantalla usando el mismo formato que los procesos Registrador, y repite el
                ciclo de extracción/muestra.*/
            if (block.is_valid_solution) {
                printf("Id : %03d\nWinner : %d\nTarget : %08ld", block.id, block.winner, block.target);
                printf("\nSolution : %08ld (validated)\nVotes : %d/%d\nWallets :", 
                    block.solution, block.posVotes, block.totalVotes);
            } else {
                printf("Id : %03d\nWinner : %d\nTarget : %08ld", block.id, block.winner, block.target);
                printf("\nSolution : %08ld (rejected)\nVotes : %d/%d\nWallets :", 
                    block.solution, block.posVotes, block.totalVotes);
            }

            while (i < block.size) {
                printf(" %d:%d", block.wallets[i].pid, block.wallets[i].monedas);
                i++;
            }
            printf("\n\n");
        }

        /* Cuando se extrae el bloque de finalización, libera todos los recursos y termina con
            código de salida EXIT_SUCCESS si todo fue correcto, o EXIT_FAILURE en caso de error.*/
        munmap(buffer, sizeof(circular_buffer));
        exit(EXIT_SUCCESS);
    }
    return 0;
}