/**
 * @file miner.c
 * @author Diego Gómez y Pablo Gómez.
 * @brief Implementación de la función monitor.
 * @version 1.0
 * @date 2025-02-18
 */
#include "miner.h"
#include "monitor.h"

#include <stdatomic.h>
#include <stdbool.h>

atomic_int flag = ATOMIC_VAR_INIT(0);
static volatile sig_atomic_t  got_usr1 = 0, got_usr2 = 0, got_alrm = 0, got_int = 0;

void handler(int sig){
    if (sig == SIGUSR1){
        got_usr1 = 1; /* Señal de inicio de ronda */
    } else if (sig == SIGUSR2){
        got_usr2 = 1; /* Señal de solución encontrada y comienzo de votación */
    } else if (sig == SIGALRM){
        got_alrm = 1; /* Señal de fin de tiempo de ejecución */
    } else if (sig == SIGINT){
        got_int  = 1; /* Señal de interrupción */
    }
}


int main(int argc, char **argv) {
    int i, j=0, n_seconds, n_threads, fd_pipe[2], state, fd, fd_read;
    char file[128];
    pid_t pid;
    MiningData block;

    /* Este proceso arranca a través de un programa que se debe ejecutar con dos parámetros:
        ./ miner <N_SECONDS> <N_THREADS> donde miner es el nombre del ejecutable, <N_SECONDS> 
        es el número de segundos que va a ejecutar el proceso y <N_THREADS> es el número de 
        hilos que se van a utilizar.*/
        
    if (argc != 3){
        printf("Formato: %s <N_SECONDS> <N_THREADS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    n_seconds = atoi(argv[1]);
    if (n_seconds < 1){
        printf("El numero de segundos tiene que ser mayor o igual a 1\n");
        exit(EXIT_FAILURE);
    }

    n_threads = atoi(argv[2]);
    if (n_threads > MAX_THREADS || n_threads < 1){
        printf("El numero de threads tiene que ser mayor o igual a 1 y menor o igual a %d\n", MAX_THREADS);
        exit(EXIT_FAILURE);
    }

    /* Se crea un pipe para comunicar los bloques al Registrador */
    if (pipe(fd_pipe)== -1){
        perror("Apertura pipe");
        exit(EXIT_FAILURE);
    }

    /* Se hace fork */
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);

    }
    if (pid) { /* Miner */
        pid = getpid();

        /* Cierra el extremo de lectura */
		close(fd_pipe[0]);

        mining_process(pid, n_threads, n_seconds, fd_pipe[1]);

        wait(&state); 
        if (WEXITSTATUS(state)) {
            printf("Registrator failed\n");
        }

        /* Cierra el extremo de escritura */
        close(fd_pipe[1]);

        exit(EXIT_SUCCESS);

    } else { /* El proceso Registrador arranca como hijo de un proceso Minero. */

        /* Se cierra el descriptor de escritura del pipe */
        close(fd_pipe[1]);
    
        /* Se construye el nombre del archivo con el formato REG_FILE_pid.dat */
        snprintf(file, sizeof(file), "%s_%d.dat", REG_FILE, getppid());
        
        /* Se intenta crear y abrir el archivo de forma exclusiva */
        fd = open(file, O_WRONLY | O_APPEND | O_CREAT | O_EXCL, 0666);
        if (fd == -1) {
            if (errno == EEXIST) {
                /* Si ya existe, se abre el archivo sin el flag EXCL */
                fd = open(file, O_WRONLY | O_APPEND | O_CREAT, 0666);
                if (fd == -1) {
                    perror("Error al abrir el archivo");
                    exit(EXIT_FAILURE);
                }
            } else {
                perror("Error al crear el archivo");
                exit(EXIT_FAILURE);
            }
        }
    
        /* Registro de bloque: Mientras la tubería que lo conecta con Minero se mantenga abierta, lee los
            bloques, y los escribe usando la función dprintf en un fichero cuyo nombre dependa del PID de
            su proceso padre.*/
        while (1) {
            i = 0;
            fd_read = read(fd_pipe[0], &block, sizeof(block));
            if (fd_read == -1) {
                perror("Error en read desde el pipe");
                exit(EXIT_FAILURE);
            }
            /* Si no se leyeron datos o se recibió un bloque terminador, se sale del bucle */
            if (fd_read == 0 || block.solution == -1) {
                break;
            }
        
            /* Escribe la cabecera del bloque */
            if (block.posVotes >= block.totalVotes / 2) {
                dprintf(fd, "Id : %03d\nWinner : %d\nTarget : %08ld", j, block.winner, block.target);
                dprintf(fd, "\nSolution : %08ld (validated)\nVotes : %d/%d\nWallets :", block.solution, block.posVotes, block.totalVotes);
            } else {
                dprintf(fd, "Id : %03d\nWinner : %d\nTarget : %08ld", j, block.winner, block.target);
                dprintf(fd, "\nSolution : %08ld (rejected)\nVotes : %d/%d\nWallets :", block.solution, block.posVotes, block.totalVotes);
            }
        
            /* Imprime la lista de wallets */
            while (i < block.size) {
                dprintf(fd, " %d:%d", block.wallets[i].pid, block.wallets[i].monedas);
                i++;
            }
            dprintf(fd, "\n\n");
            j++;
        }
    
        /* Se cierra el archivo y el extremo de lectura del pipe */
        close(fd);
        close(fd_pipe[0]);
        exit(EXIT_SUCCESS);
    }
}

int mining_process(pid_t self, int n_threads, int n_seconds, int fd_pipe){
    struct sigaction sigusr1, sigusr2, sigint, sigalarm;
    MiningData *buffer;
    pthread_t threads[MAX_THREADS];
    int fd_shm, i, range_per_thread, last;
    void *res = NULL, *thread_result = NULL;

    MiningData tmp;
    Range range[MAX_THREADS];
    mqd_t mq;
    sem_t *sem;
    
    /* Inicializar las estructuras sigaction a cero*/
    memset(&sigusr1, 0, sizeof(sigusr1));
    memset(&sigusr2, 0, sizeof(sigusr2));
    memset(&sigint, 0, sizeof(sigint));
    memset(&sigalarm, 0, sizeof(sigalarm));

    /* handler se asigna a SIGUSR1. */
    sigusr1.sa_handler = handler;

    /* Llenar la máscara de señales de 'sigusr1' para bloquear todas las señales 
        durante la ejecución de su manejador.
    */
    sigfillset(&sigusr1.sa_mask);

    /* Se elimina SIGUSR1 de la máscara para permitir que dicha señal se reciba, 
        luego se configura la acción para SIGUSR1. */   
    sigdelset(&sigusr1.sa_mask, SIGUSR1);
    if (sigaction(SIGUSR1, &sigusr1, NULL) < 0){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Se elimina SIGINT de la máscara para permitir que dicha señal se reciba, 
        luego se configura la acción para SIGINT. */   
    sigdelset(&sigusr1.sa_mask, SIGINT);
    if (sigaction(SIGINT, &sigint, NULL) < 0){
        perror("sigaction");
        exit(EXIT_FAILURE); 
    }

    /* Se elimina SIGALRM de la máscara para permitir que dicha señal se reciba, 
        luego se configura la acción para SIGALRM. */ 
    sigdelset(&sigusr1.sa_mask, SIGALRM);
    if (sigaction(SIGALRM, &sigalarm, NULL) < 0){
        perror("sigaction");
        exit(EXIT_FAILURE); 
    }

    /* handler se asigna a SIGUSR2. */
    sigusr2.sa_handler = handler;

    /* Llenar la máscara de señales de 'sigusr2' para bloquear todas las señales 
        durante la ejecución de su manejador.
    */
    sigfillset(&sigusr2.sa_mask);

    /* Se elimina SIGUSR2 de la máscara para permitir que dicha señal se reciba, 
        luego se configura la acción para SIGUSR2. */   
    sigdelset(&sigusr2.sa_mask, SIGUSR2);
    if (sigaction(SIGUSR2, &sigusr2, NULL) < 0){
        perror("sigaction");
        exit(EXIT_FAILURE); 
    }

    /* Se elimina SIGINT de la máscara para permitir que dicha señal se reciba, 
        luego se configura la acción para SIGINT. */  
    sigdelset(&sigusr2.sa_mask, SIGINT);
    if (sigaction(SIGINT, &sigint, NULL) < 0){
        perror("sigaction");
        exit(EXIT_FAILURE); 
    }

    /* Se elimina SIGALRM de la máscara para permitir que dicha señal se reciba, 
        luego se configura la acción para SIGALRM. */ 
    sigdelset(&sigusr2.sa_mask, SIGALRM);
    if (sigaction(SIGALRM, &sigalarm, NULL) < 0){
        perror("sigaction");
        exit(EXIT_FAILURE); 
    }

    /* handler se asigna a SIGINT. */
    sigint.sa_handler = handler;

    /* Llenar la máscara de señales de 'sigint' para bloquear todas las señales 
        durante la ejecución de su manejador.
    */
    sigfillset(&sigint.sa_mask);

    /* Se elimina SIGINT de la máscara para permitir que dicha señal se reciba, 
        luego se configura la acción para SIGINT. */    
    sigdelset(&sigint.sa_mask, SIGINT);
    if (sigaction(SIGINT, &sigint, NULL) < 0){
        perror("sigaction");
        exit(EXIT_FAILURE); 
    }

    /* handler_alrm se asigna a SIGALRM. */
    sigalarm.sa_handler = handler;

    /* Llenar la máscara de señales de 'sigint' para bloquear todas las señales 
        durante la ejecución de su manejador.
    */
    sigfillset(&sigalarm.sa_mask);

    /* Se elimina SIGALRM de la máscara (aun cuando se use la misma estructura sigint.sa_mask)
       para permitir que la señal SIGALRM sea recibida y se configure su acción. */
    sigdelset(&sigalarm.sa_mask, SIGALRM);
    if (sigaction(SIGALRM, &sigalarm, NULL) < 0){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Conexión a la memoria compartida */
    fd_shm = shm_open(SHM_MINER, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd_shm == -1) {
        if (errno == EEXIST) {
            if ((fd_shm = shm_open(SHM_MINER, O_RDWR, S_IRUSR | S_IWUSR)) == -1) {
                perror("Error opening the shared memory segment");
                exit(EXIT_FAILURE);
            }
            else {
                perror("Shared memory segment open\n");
                flag ++;
            }
        } else {
            perror("Error creating the shared memory segment\n");
            exit(EXIT_FAILURE);
        }
    } else {
        perror("Shared memory segment created\n");
        if (ftruncate (fd_shm , sizeof(MiningData)) == -1) {
            perror("ftruncate");
            close(fd_shm);
            exit(EXIT_FAILURE);
        }
    }

    buffer = mmap(NULL, sizeof(MiningData), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (buffer == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_MINER);
        exit(EXIT_FAILURE); 
    }

    /* Registro de minero y arranque de primera ronda*/
    if (!flag){
        buffer = initMinerData(buffer);
        if (buffer == NULL){
            exit(EXIT_FAILURE);
        }

        buffer->target = 0;

        /* Se añade primer minero */
        if (addMiner(buffer, self)){
            printf("El sistem no acepta mas mineros\n"); 
            exit(EXIT_SUCCESS);
        }

        /* Notifica con SIGUSR1 a todos para arrancar ronda */
        for (i = 0; i < buffer->size; i++){
            if(kill(buffer->wallets[i].pid, SIGUSR1) == -1){
                perror("kill");
                exit(EXIT_FAILURE);  
            }
        }
    } else{
        /* Se añade minero adicional*/
        if (addMiner(buffer, getpid())){ 
            printf("El sistem no acepta mas mineros\n"); 
            exit(EXIT_SUCCESS);
        }
    }

    /* Se abre cola de mensajes y semáforo para comunicación con monitor */
    if ((mq = mq_open(MQ_NAME,O_CREAT |O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR, &attributes)) == (mqd_t)-1) {
        if (errno == EEXIST){
            if ((mq = mq_open(MQ_NAME, O_WRONLY, S_IRUSR | S_IWUSR, &attributes)) == -1) {
                perror("mq_open");
                exit(EXIT_FAILURE);
            }

        }
        else{
            perror("mq_open");
            exit(EXIT_FAILURE);
        }
    }

    if (( sem = sem_open ( SEM_NAME , O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR , 0)) == SEM_FAILED ) {
        if (errno == EEXIST){
            if ((sem = sem_open ( SEM_NAME , O_RDWR, S_IRUSR | S_IWUSR , 0)) == SEM_FAILED) {
                perror("sem_open(EEXIST)");
                exit(EXIT_FAILURE);
            }
        }
        else{
            perror("sem_open");
            exit(EXIT_FAILURE);
        }
    }
   

    range_per_thread = (POW_LIMIT - 1)/ n_threads;
    alarm(n_seconds);

    /* Bucle de minado, votación y envío de bloques */
    while (!got_alrm && !got_int){
        /* Espera SIGUSR1 para nueva ronda */
        if(!got_usr1){
            sigsuspend(&sigusr1.sa_mask);
        }

        got_usr1 = 0;
        res = NULL;

        /* Divide espacio e inicia hilos */
        for (i = 0; (i < n_threads) && (!got_usr2) && (atomic_load(&flag) == 0); i++){
            /* Inicio del subrango. */
            range[i].ini = i * range_per_thread;

            /* Fin del subrango */
            if (i == (n_threads - 1)){
                range[i].end = POW_LIMIT - 1;
            } else {
                range[i].end = range_per_thread * (i + 1) - 1;
            }

            /* Asigna el valor objetivo que los hilos deben buscar. */
            range[i].target_value = buffer->target;

            /* Cada uno ejecuta la función miningThread */
            if (pthread_create(&threads[i], NULL, miningThread, range + i) != 0){
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        }

        /* Espera a que terminen todos los hilos y recoge la posible solución */
        for (i -= 1, thread_result = NULL; i >=0; i--) {
            if (pthread_join(threads[i], &thread_result) != 0){
                perror("pthread_join");
                exit(EXIT_FAILURE);
            }
        
            if (thread_result != NULL){
                res = thread_result;
            }
        }
        
        /* Si se encontró una solución, se intenta marcar al ganador de forma atómica */
        if (atomic_load(&flag)) {
            sem_wait(&buffer->sem_mutex);
            if (buffer->winner == -1) {
                buffer->winner   = self;
                buffer->solution = (intptr_t)res;
                atomic_store(&flag, 1); 
            } else {
                atomic_store(&flag, 0);
            }
            sem_post(&buffer->sem_mutex);

            /* Si se marcó la solución, se notifica a todas las wallets con SIGUSR2*/
            if (atomic_load(&flag) == 1) {
                for (i = 0; i < buffer->size; i++) {
                    if (kill(buffer->wallets[i].pid, SIGUSR2) == -1) {
                        perror("kill");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }

        /* Reiniciamos las banderas para la siguiente iteración*/
        atomic_store(&flag, 0);
        got_usr2 = 0;
        got_usr1 = 0;

        /* Se decide si este proceso es el ganador o el perdedor */
        if (buffer->winner == self){
            /* Proceso ganador */
            voteWinner(buffer, self);

            sem_wait(sem);
            if (mq_send(mq, (char *)buffer, sizeof(MiningData), 1) == -1) {
                perror("mq_send");
                exit(EXIT_FAILURE);
            }
            sem_post(sem);

            /* Actualiza el target con la solución encontrada y reinicia el ganador */
            if (write(fd_pipe, buffer, sizeof(MiningData)) == -1){
                perror("write");
                exit(EXIT_FAILURE);  
            }

            sem_wait(&buffer->sem_mutex);
            buffer->target = buffer->solution;
            buffer->winner = -1;
            buffer->solution = -1;
            sem_post(&buffer->sem_mutex);


            /* Notifica a las demás wallets mediante SIGUSR1 */
            for (i = 0;i < buffer->size; i++){ 
                if(kill(buffer->wallets[i].pid, SIGUSR1) == -1){
                    perror("kill");
                    exit(EXIT_FAILURE);  
                }
            }
        }
        else{
            /* Proceso perdedor */
            voteLoser(buffer, sigusr2.sa_mask);
            if (write(fd_pipe, buffer, sizeof(MiningData)) == -1){
                perror("write");
                exit(EXIT_FAILURE);  
            }
        }
    }
    /* Se guarda el estado final según el tamaño previo a eliminar el minero. */
    last = buffer->size == 1;

    /* Se elimina al minero */
    removeMiner(buffer, self);

    if (last){ /* Ultimo minero */

        buffer->target = -1;
        buffer->solution = -1;

        if (write(fd_pipe, buffer, sizeof(MiningData)) == -1){
            perror("write");
            exit(EXIT_FAILURE);  
        }

        sem_wait(sem);
        if (mq_send(mq, (char *)buffer, MSG_SIZE, 1) == -1) {
            perror("mq_send");
            exit(EXIT_FAILURE);
        }
        sem_post(sem);

        munmap(buffer, sizeof(MiningData));
        sem_unlink(SEM_NAME);
        shm_unlink(SHM_MINER);
        mq_close(mq); 
        sem_close(sem); 

    } else { /* No es el último minero. */
        /* Si el minero actual es el ganador, se envía SIGUSR1 a todas las wallets.*/
        if (buffer->winner == self){
            for (i = 0; i < buffer->size; i++){ 
                if(kill(buffer->wallets[i].pid, SIGUSR1) == -1){
                    perror("kill");
                    exit(EXIT_FAILURE);  
                }
            }
        } 

        /* Se prepara una estructura temporal para indicar que ya no hay solución. */
        tmp.solution = -1;
        if (write(fd_pipe, &tmp, sizeof(MiningData)) == -1){
            perror("Write pipe ganador");
            exit(EXIT_FAILURE);  
        }
    }
    return 1;
}

/* Función de minado ejecutada en un hilo.
   Recorre un rango de valores buscando un candidato cuya función de hash coincida con target_value.
   Si lo encuentra, marca la solución encontrada y sale retornando el valor encontrado. */
void *miningThread(void *range){
    long int i;
    Range *r = (Range *) range;

    for (i = r->ini; i < r->end && atomic_load(&flag) == 0 && !got_usr2; i++){
        if (pow_hash(i) == r->target_value){
            atomic_store(&flag, 1);
            pthread_exit((void *)(uintptr_t)i);
        }

    }
    pthread_exit(NULL);
}

/* Envía SIGUSR2 a todos (menos al emisor) */
static void notify_all_except(MiningData *minerData, pid_t self, int sig) {
    for (int i = 0; i < minerData->size; i++) {
        if (minerData->wallets[i].pid != self) {
            if (kill(minerData->wallets[i].pid, sig) == -1) {
                perror("kill");
                exit(EXIT_FAILURE);
            }
        }
    }
}

/* Factoriza la actualización de votos (positivo o sólo total) */
static void record_vote(MiningData *minerData, bool positive) {
    sem_wait(&minerData->sem_mutex);
    /* Voto positivo: incrementa votos positivos y totales de forma atómica */
    if (positive) {
        minerData->posVotes++;
    }
    /* Voto negativo: sólo incrementa el total de votos de forma atómica */
    minerData->totalVotes++;
    sem_post(&minerData->sem_mutex);
}

void voteWinner(MiningData *minerData, pid_t pid_miner) {
    int index = -1;
    minerData->totalVotes = minerData->posVotes = 0;

    /* Notifica a todos los demás */
    notify_all_except(minerData, pid_miner, SIGUSR2);

    /* Encontrar mi índice para la recompensa */
    for (int i = 0; i < minerData->size; i++) {
        if (minerData->wallets[i].pid == pid_miner) {
            index = i;
            break;
        }
    }

    /* Emitir mi voto */
    bool valid = (pow_hash(minerData->solution) == minerData->target);
    record_vote(minerData, valid);

    /* Espera al resto (o timeout) */
    for (int try = 0; minerData->totalVotes < minerData->size && try < MAX_TRY; try++) {
        usleep(500);
    }

    /* Premio si mayoría valida */
    if (minerData->posVotes >= minerData->totalVotes/2 && index != -1) {
        sem_wait(&minerData->sem_mutex);
        minerData->wallets[index].monedas++;
        sem_post(&minerData->sem_mutex);
    }
}

void voteLoser(MiningData *minerData, const sigset_t sigmask_usr2) {
    /* Si aún no ha llegado la señal SIGUSR2 que inicia la votación, 
        espera bloqueando hasta recibirla */
    if (!got_usr2) {
        sigsuspend(&sigmask_usr2);
    }

    /* Reinicia la bandera para futuras rondas */
    got_usr2 = 0;

    /* Comprueba si la solución actual valida el objetivo */
    bool valid = (pow_hash(minerData->solution) == minerData->target);
    record_vote(minerData, valid);
}
