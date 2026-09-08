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

int main (int argc, char **argv){
    struct sigaction sigint, sigalarm;
    int n_segs, n_proc, i;
    pid_t pid = 1;
    FILE *file;
    sigset_t suspendSet;

    /* Validación de argumentos */
    if (argc != 3){
		printf("Debes usar: ./voting <N_PROCS> <N_SECS>\n");
		exit(EXIT_FAILURE);
	}

    /* Convertir el primer argumento a entero (número de procesos).*/
    n_proc = atoi(argv[1]);
    if (n_proc <= 0){ 
        printf("El numero de procesos tiene que ser positivo y mayor que cero\n");
        exit(EXIT_FAILURE);
    }

    /* Convertir el segundo argumento a entero (número de segundos).*/
    n_segs = atoi(argv[2]);
    if (n_segs <= 0){
        printf("El numero de segundos tiene que ser positivo y mayor que cero\n");
        exit(EXIT_FAILURE);
    }

    /* Inicializar las estructuras sigaction a cero*/
    memset(&sigint, 0, sizeof(sigint));
    memset(&sigalarm, 0, sizeof(sigalarm));

    /* handler se asigna a SIGINT. */
    sigint.sa_handler = handler_int;

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
    sigalarm.sa_handler = handler_alarm;

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

    /* Abrir el archivo definido por FILE_NAME en modo escritura.*/
    file = fopen(FILE_NAME, "w");
    if (file == NULL){
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* Bucle para crear 'n_proc' procesos hijos. */    
    for (i = 0; i < n_proc; i++) {
        pid = fork();
        if (pid < 0) { /*Error en fork*/
            perror("Fork");
            fclose(file);
            exit(EXIT_FAILURE);
        } 
        else if (pid == 0) { /*Proceso hijo*/
            /* El proceso hijo ejecuta la función votante */
            signal(SIGINT, SIG_IGN);
            votante(n_proc, n_segs);
            exit(EXIT_SUCCESS);
        } 
        else { /*Proceso padre*/
            /* El proceso padre escribe el PID del proceso hijo en el archivo.*/
            if (fwrite(&pid, sizeof(pid), 1, file) <= 0) {
                perror("write");
                fclose(file);
                exit(EXIT_FAILURE);
            }
        }
    
    }

    /* Se cierra el archivo después de registrar todos los PIDs. */
    fclose(file);

    /* Reabrir el archivo en modo lectura para poder enviar una señal a cada proceso.
       Esto se hace para iniciar la votación, enviando SIGUSR1 a cada proceso registrado. */    
    file = fopen(FILE_NAME, "r");
    if (file == NULL){
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* Bucle para leer cada PID del archivo y enviar la señal SIGUSR1.
        SIGUSR1 indica a los procesos que deben comenzar la votación. */
    for (i = 0; i < n_proc; i++){
        if (fread(&pid, sizeof(pid), 1, file) == -1){
            perror("read");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        if(kill(pid, SIGUSR1) == -1){ /*Empieza la votacion*/
            perror("kill");
            fclose(file);
            exit(EXIT_FAILURE);  
        }
    }

    /* Se cierra el archivo después de enviar las señales. */
    fclose(file);

    /* Se programa una alarma que se activará después de 'n_segs' segundos */
    alarm(n_segs);

    /* Se suspende la ejecución del proceso principal, bloqueando todas las señales definidas en sigint.sa_mask.
       Esta suspensión permite esperar hasta que se reciba una señal (SIGINT o SIGALRM) que interrumpa la suspensión. */
    sigemptyset(&suspendSet);
    sigsuspend(&suspendSet);

    return 0;
}