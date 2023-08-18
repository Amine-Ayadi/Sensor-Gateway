/**
 * \author Amine Ayadi
 */

#ifndef _SBUFFER_H_
#define _SBUFFER_H_

#include "config.h"
#include <pthread.h>
#include <stdatomic.h>

#define SBUFFER_FAILURE -1
#define SBUFFER_SUCCESS 0
#define SBUFFER_NO_DATA 1
#define SBUFFER_HAS_DATA 2

typedef struct sbuffer_node {
    struct sbuffer_node *next;  /**< a pointer to the next node*/
    sensor_data_t data;         /**< a structure containing the data */
    int managed;
    int stored;
} sbuffer_node_t;

/**
 * a structure to keep track of the buffer
 */
struct sbuffer {
    sbuffer_node_t *head;       /**< a pointer to the first node in the buffer */
    sbuffer_node_t *tail;       /**< a pointer to the last node in the buffer */
    /***MUTEXES***/
    pthread_mutex_t fifo_mutex;
    pthread_mutex_t sbuffer_mutex;
    /***CONDITIONS***/
    pthread_cond_t buffer_no_data;
    pthread_cond_t buffer_has_data1;
    pthread_cond_t buffer_has_data2;
    /***FLAGS***/
    atomic_int exit_flag;
};

char* get_time_format(time_t time);

typedef struct sbuffer sbuffer_t;

/**
 * Allocates and initializes a new shared buffer
 * \param buffer a double pointer to the buffer that needs to be initialized
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_init(sbuffer_t **buffer);
int buffer_size(sbuffer_t* buffer);
int buffer_has_data(sbuffer_t* buffer);
sensor_data_t* sbuffer_get_first(sbuffer_t *buffer);
sensor_data_t* sbuffer_get_next(sbuffer_t* buffer);
void log_event(const char* event_message);
void log_msg(const char* format, ...);
/**
 * All allocated resources are freed and cleaned up
 * \param buffer a double pointer to the buffer that needs to be freed
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_free(sbuffer_t **buffer);

/**
 * Removes the first sensor data in 'buffer' (at the 'head') and returns this sensor data as '*data'
 * If 'buffer' is empty, the function doesn't block until new sensor data becomes available but returns SBUFFER_NO_DATA
 * \param buffer a pointer to the buffer that is used
 * \param data a pointer to pre-allocated sensor_data_t space, the data will be copied into this structure. No new memory is allocated for 'data' in this function.
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data);

/**
 * Inserts the sensor data in 'data' at the end of 'buffer' (at the 'tail')
 * \param buffer a pointer to the buffer that is used
 * \param data a pointer to sensor_data_t data, that will be copied into the buffer
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occured
*/
int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data);

#endif  //_SBUFFER_H_
