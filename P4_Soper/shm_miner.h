#ifndef _SHM_MINER_H_
#define _SHM_MINER_H_

#include <semaphore.h>
#include <sys/types.h> 

#define MAX_MINER 100

/**
 * @brief Estructura que representa una wallet/minero.
 *
 * Contiene el PID del proceso del minero y la cantidad de monedas acumuladas.
 */
typedef struct _wallet {
    pid_t pid;   /* PID del proceso del minero */
    int monedas; /* Monedas acumuladas por el minero */
} Wallet;

/**
 * @brief Estructura de datos compartidos para el minero.
 *
 * Contiene semáforo para exclusión mutua, información del desafío actual
 * (target y solution) y resultados de la votación, así como la lista de wallets.
 */
typedef struct _miningData {
    sem_t sem_mutex; /* Semáforo para proteger el acceso a MiningData */
    long target;     /* Valor objetivo (target) que los mineros deben resolver */
    long solution;   /* Valor de la solución encontrada */
    int is_valid_solution;  /* Indicador de validez de la solución (1 válida, 0 inválida) */
    pid_t winner;    /* PID del minero ganador */
	int totalVotes;  /* Número total de votos en la ronda */
	int posVotes;    /* Número de votos positivos */
    int size;        /* Número actual de wallets registrados */
    Wallet wallets[MAX_MINER]; /* Array de wallets de los mineros */
} MiningData;

/**
 * @brief Inicializa la estructura MiningData en memoria compartida.
 *
 * Configura el semáforo y los valores iniciales (target = 0, solution = -1,
 * winner = -1, votos a cero, size = 0).
 *
 * @param minerData Puntero a la estructura pre-alocada.
 * @return Puntero a la misma estructura para encadenamiento.
 */
MiningData *initMinerData(MiningData *minerData);

/**
 * @brief Destruye y libera recursos asociados a MiningData.
 *
 * Cierra el semáforo y desbloquea la memoria compartida.
 *
 * @param minerData Puntero a la estructura a destruir.
 */
void destroyMinerData(MiningData *minerData);

/**
 * @brief Añade un nuevo minero (wallet) a MiningData.
 *
 * Registra el PID del nuevo minero en la lista de wallets.
 *
 * @param minerData Puntero a la estructura compartida.
 * @param pid PID del proceso del minero a añadir.
 * @return 0 si se añadió correctamente, -1 si no hay espacio.
 */
int addMiner(MiningData *minerData, pid_t pid);

/**
 * @brief Elimina un minero (wallet) de MiningData.
 *
 * Quita el PID especificado de la lista de wallets y ajusta el tamaño.
 *
 * @param minerData Puntero a la estructura compartida.
 * @param pid PID del proceso del minero a eliminar.
 */
void removeMiner(MiningData *minerData, pid_t pid);

#endif