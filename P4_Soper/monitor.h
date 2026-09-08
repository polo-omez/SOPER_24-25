#ifndef _MONITOR_H
#define _MONITOR_H

#include <mqueue.h>
#include "shm_miner.h"

#define SHM_NAME "/shm"
#define MQ_NAME "/mq"
#define SEM_NAME "/sem"
#define MSG_SIZE 100

/** 
 * Se usa un buffer circular de 5 bloques en el segmento de memoria compartida. 
 */
struct mq_attr attributes = {.mq_flags = 0,
                    .mq_maxmsg = 5,
                    .mq_curmsgs = 0,
                    .mq_msgsize = sizeof(MiningData)};

#endif