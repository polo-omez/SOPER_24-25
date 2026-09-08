/**
 * @file monitor.h
 * @author Diego Gómez y Pablo Gómez.
 * @brief Definición de la estructura y función para monitor.
 * @version 1.0
 * @date 2025-02-18
 */

#ifndef _MONITOR_H
#define _MONITOR_H


/**
 * @brief the function decides whether the solution found by the miner is acepted or not
 *
 * @param target target value
 * @param solution solution found by the miner
 * @return 1 if the solution is acepted 0 if rejected
 */
long int monitor(long int target, long int solution);

/**
 * @brief Ejecuta las rondas del Monitor.
 *
 * Lee en cada ronda el dato objetivo y la solución a través de la tubería (pipe1),
 * verifica la solución usando la función monitor, y envía la respuesta de vuelta 
 * a través de la tubería (pipe2).
 *
 * @param rounds Número de rondas a ejecutar.
 * @param pipe1_read Descriptor de lectura de pipe1.
 * @param pipe2_write Descriptor de escritura de pipe2.
 */
void ejecutarRondasMonitor(int rounds, int pipe1_read, int pipe2_write);

#endif
