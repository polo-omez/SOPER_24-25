#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#include "shm_miner.h"
#include <string.h>

MiningData *initMinerData(MiningData *minerData) {
    if (minerData == NULL)
        return NULL;

    minerData->winner = -1;
    minerData->solution = -1;
    minerData->totalVotes = 0;
    minerData->posVotes = 0;
    minerData->size = 0;

    if (sem_init(&minerData->sem_mutex, 1, 1) != 0) {
        perror("sem_init");
        free(minerData);
        return NULL;
    }
    return minerData;
}

void destroyMinerData(MiningData *minerData) {
    if (minerData == NULL)
        return;
    
    sem_destroy(&minerData->sem_mutex);
}

int addMiner(MiningData *minerData, pid_t pid) {
    if (minerData->size >= MAX_MINER) {
        return -1;
    }
    minerData->wallets[minerData->size].pid = pid;
    minerData->size++;
    return 0;
}

void removeMiner(MiningData *minerData, pid_t pid) {
    int i;
    for (i = 0; i < minerData->size; i++) {
        if (minerData->wallets[i].pid == pid) {
            memmove(&minerData->wallets[i], &minerData->wallets[i + 1], (minerData->size - i - 1) * sizeof(Wallet));
            minerData->size--;
            return;
        }
    }
}

