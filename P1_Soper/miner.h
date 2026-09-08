/**
 * @file miner.h
 * @author Diego Gómez y Pablo Gómez.
 * @brief Definición de la estructura y función para miner.
 * @version 1.0
 * @date 2025-02-18
 */

#ifndef _MINER_H
#define _MINER_H

/**
 * @brief Función ejecutada por cada hilo de miner.
 *
 * Cada hilo explora un rango de valores buscando un número cuyo hash coincida
 * con un valor objetivo. Si un hilo encuentra una solución, la comunica
 * mediante pipe_exit y detiene la ejecución de otros hilos estableciendo
 * la variable global flag a 1.
 *
 * @param range Puntero a una estructura Range que define el rango de búsqueda del hilo.
 * @return void* Devuelve el valor encontrado si se encuentra una solución, de lo contrario NULL.
 */
void *miner(void *range);


/**
 * @brief Ejecuta las rondas de minería creando hilos y coordinando la comunicación con el Monitor.
 *
 * @param rounds Número de rondas a ejecutar.
 * @param n_threads Número de hilos a utilizar.
 * @param target Valor objetivo inicial a buscar.
 * @param pipe_write Descriptor de escritura de la tubería (pipe1[1]) para enviar datos al Monitor.
 * @param pipe_read Descriptor de lectura de la tubería (pipe2[0]) para recibir la respuesta del Monitor.
 * @return int Estado devuelto por el Monitor (1 si la solución fue aceptada, 0 en caso contrario).
 */
int ejecutarRondasMiner(int rounds, int n_threads, long target, int pipe_write, int pipe_read);

#endif