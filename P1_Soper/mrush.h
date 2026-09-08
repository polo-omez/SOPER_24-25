#ifndef _MRUSH_H
#define _MRUSH_H

#define MAX_THREADS 100 /* Número máximo de hilos */

/**
 * @struct Range
 * @brief Estructura que define el rango de búsqueda de cada hilo.
 *
 * La estructura almacena los valores de inicio y fin del rango de búsqueda,
 * el valor objetivo que se está buscando, y un descriptor de archivo para
 * la comunicación entre procesos a través de pipes.
 */
typedef struct {
    long int ini;           /* Valor inicial del rango de búsqueda. */
    long int end;           /* Valor final del rango de búsqueda. */
    long int target_value;  /* Es el valor objetivo que los hilos deben buscar. */
    long int pipe_exit;     /* Es fd[1], el extremo donde los hilos escriben los resultados.*/
} Range;

#endif