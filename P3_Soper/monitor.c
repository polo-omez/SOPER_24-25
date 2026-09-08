/**
 * @file monitor.c
 * @author Diego Gómez y Pablo Gómez.
 * @brief Implementación de la función monitor.
 * @version 1.0
 * @date 2025-02-18
 */
#include "monitor.h"
#include "pow.h"
#include "buffer.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdatomic.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

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
        printf("Solution accepted: %08ld --> %08ld\n", target, solution);
        return 1;
    } else{
        /* En caso contrario, la solución es rechazada. */
        printf("Solution rejected: %08ld !-> %08ld\n", target, solution);
        return 0;
    }
}


int monitor_mq(long int target, long int solution) {
	if (pow_hash(solution) == target) {
		return 1;
	}
	else {
		return 0;
	}
}


int main(int argc, char **argv) {
    int lag, fd_shm, flag_monitor = 0, queue;
    circular_buffer *buffer = NULL;
    block block;

    block.solution = 0;
    block.target = 0;
    block.is_valid_solution = 0;


    if (argc != 2){
        printf("Formato: %s <LAG>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    lag = atoi(argv[1]);
    if (lag < 0){
        printf("El lag tiene que que ser un numero positivo\n");
        exit(EXIT_FAILURE);
    }

    /* This is only a code fragment, not a complete program... */
    fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd_shm == -1) {
        if (errno == EEXIST) {
            fd_shm = shm_open(SHM_NAME, O_RDWR, 0);
            if (fd_shm == -1) {
                perror("Error opening the shared memory segment");
                exit(EXIT_FAILURE);
            } else {
                perror("Shared memory segment open\n");
                flag_monitor++;
            }
        } else {
            perror("Error creating the shared memory segment\n");
            exit(EXIT_FAILURE);
        }
    } else {
        perror("Shared memory segment created\n");
    }

    if (!flag_monitor){

        printf("[%d] Checking blocks...\n", getpid());

        /*truncate*/  /*Espacio que quieremos usar en la memoria*/
        if (ftruncate(fd_shm, sizeof(circular_buffer)) == -1) {
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

        /* INICIALIZAR BUFFER */
        buffer = buffer_init(buffer);
        if (buffer == NULL){
            exit(EXIT_FAILURE);
        }

        queue = mq_open(
            MQ_NAME,
            O_CREAT | O_RDONLY, /* This process is going to receive messages. */
            S_IRUSR | S_IWUSR,  /* The user can read and write. */
            &attributes);
      
        if (queue == (mqd_t)-1) {
            perror("mq_open");
            exit(EXIT_FAILURE);
        }

        /* PRODUCTOR */
        /*
            Down (sem_empty);
            Down (sem_mutex);
            AñadirElemento();
            Up(sem_mutex);
            Up(sem_fill);
        */
        while (block.solution != -1) {
            if (mq_receive(queue, (char *)&buffer->msg, MSG_MAX, NULL) == -1) {
                perror("mq_receive");
                block.solution = -1;
                /*sem_wait(&buffer->sem_empty);*/
                sem_wait(&buffer->sem_mutex);
                buffer_addElem(buffer, block);
                sem_post(&buffer->sem_mutex);
                /*sem_post(&buffer->sem_fill);*/
                exit(EXIT_FAILURE);
            } else {
                char *target_str = strtok(buffer->msg, ";");
                char *solution_str = strtok(NULL, ";");

                if (target_str == NULL || solution_str == NULL) {
                    perror("Error parsing message");
                    exit(EXIT_FAILURE);
                }

                block.target = atol(target_str);
                block.solution = atol(solution_str);
                block.is_valid_solution = monitor_mq(block.target, block.solution);
            }

            sem_wait(&buffer->sem_empty);
            sem_wait(&buffer->sem_mutex);
            buffer_addElem(buffer, block);
            sem_post(&buffer->sem_mutex);
            sem_post(&buffer->sem_fill);
    
            
            usleep(lag * 1000);
        }
        
        mq_close(queue);

        /* Unmapping and freeing of the shared memory */
        munmap(buffer, sizeof(circular_buffer));
        shm_unlink(MQ_NAME);

        printf("[%d] Finishing\n", getpid());
        exit(EXIT_SUCCESS);

    } else {
        printf("[%d] Printing blocks...\n", getpid());

        /* Mapping of the memory segment. */
        buffer = mmap(NULL, sizeof(circular_buffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
        close(fd_shm);
        if (buffer == MAP_FAILED) {
            perror("mmap");
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }

        /* CONSUMIDOR */
        /*
            Down (sem_fill);
            Down (sem_mutex);
            ExtraerElemento();
            Up(sem_mutex);
            Up(sem_empty); 
        */
        while (1) {
            sem_wait(&buffer->sem_fill);
            sem_wait(&buffer->sem_mutex);
            block = buffer_extractElem(buffer);
            sem_post(&buffer->sem_mutex);
            sem_post(&buffer->sem_empty);
    
            /* Revisar si el bloque recibido es el bloque final de terminación */
            if (block.solution == -1) {
                break;
            }
    
            /* Verificar la solución con monitor() */
            block.is_valid_solution = monitor(block.target, block.solution);
    
            usleep(lag * 1000);
        }

        buffer_destroy(buffer);

        /* Unmapping and freeing of the shared memory */
        munmap(buffer, sizeof(circular_buffer));
        shm_unlink(SHM_NAME);

        printf("[%d] Finishing\n", getpid());
        exit(EXIT_SUCCESS);
    }

    return 0;
}

/* rm /dev/shm/smh */