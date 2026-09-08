#ifndef _MONITOR_H
#define _MONITOR_H

#include "pow.h"
#include <mqueue.h>

#define SHM_NAME "/smh"
#define MQ_NAME "/mq"
#define MSG_SIZE 100

struct mq_attr attributes = {.mq_flags = 0,
                    .mq_maxmsg = 7,
                    .mq_curmsgs = 0,
                    .mq_msgsize = MSG_SIZE};

#endif