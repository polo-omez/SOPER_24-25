/**
 * @file miner.c
 * @author Diego Gómez y Pablo Gómez.
 * @brief Implementación de la función monitor.
 * @version 1.0
 * @date 2025-02-18
 */
#include "monitor.h"
#include "miner.h"
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

int main(int argc, char **argv) {

    int rounds = 0, lag = 0, queue = 0;
    long int target = 0, solution = 0, i = 0;
    char str[MSG_SIZE] = {0}; 

    if (argc != 3){
        printf("Formato: %s <ROUNDS> <LAG>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    rounds = atoi(argv[1]);
    if (rounds < 0){
        printf("El numero de rondas tiene que que ser un numero positivo\n");
        exit(EXIT_FAILURE);
    }

    lag = atoi(argv[2]);
    if (lag < 0){
        printf("El lag tiene que que ser un numero positivo\n");
        exit(EXIT_FAILURE);
    }

    /* Creará una cola de mensajes con capacidad para 7 mensajes. */
    queue = mq_open(
        MQ_NAME,
        O_CREAT | O_WRONLY, /* This process is going to receive messages. */
        S_IRUSR | S_IWUSR,  /* The user can read and write. */
        &attributes);
  
    if (queue == (mqd_t)-1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    printf("[%d] Generating blocks...\n", getpid());

    /* Establecerá un objetivo inicial fijo para la POW (por ejemplo, 0). */
    target = 0;

    /* Para cada ronda, resolverá la POW; en el caso de que no se tenga un
    minero funcional capaz de generar una secuencia de bloques encadenados,
    se puede utilizar el fichero con la secuencia de 200 rondas proporcionado
    para falsear el minero.
     */
    while (rounds > 0) {
        solution = -1;

        for (i = 0; i < POW_LIMIT; i++) {
            if (pow_hash(i) == target) {
                solution = i;
                break;
            }
        }

        /* Enviará un mensaje por la cola de mensajes que contenga, al menos, el
        objetivo y la solución hallada.
         */
        if (solution != -1) {
            snprintf(str, MSG_SIZE, "%08ld;%08ld;", target, solution);
            
            if (mq_send(queue, str, strlen(str) + 1, 1) == -1) {
                perror("mq_send");
                mq_close(queue);
                exit(EXIT_FAILURE);
            }
        }
        
        /* Realizará una espera de LAG milisegundos. */
        usleep(lag * 1000);

        /* Establecerá como siguiente objetivo la solución anterior, y repetirá la ronda. */
        target = solution;
        rounds--;
    }

    /* Una vez terminadas las rondas, enviará un bloque especial con algún código
       que permita saber al proceso Comprobador que el sistema está finalizando. */
    snprintf(str, MSG_SIZE, "%08ld;%08ld;", (long)-1, (long)-1);
    if (mq_send(queue, str, MSG_SIZE, 1) == -1) {
        perror("mq_send");
        mq_close(queue);
        exit(EXIT_FAILURE);
    }
    
    printf("[%d] Finishing\n", getpid());


    return EXIT_SUCCESS;
}