/**
 * \author Amine Ayadi
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_
#define _GNU_SOURCE

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
//#include <sqlite3.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

#include "lib/dplist.h"
#include "lib/tcpsock.h"
#ifndef BUFFER_SIZE
    #define BUFFER_SIZE 264
#endif

#ifndef STOREMGR_TRYINIT
    #define STOREMGR_TRYINIT 3
#endif

#define FIFO_NAME "log_fifo"

typedef uint16_t sensor_id_t;
typedef double sensor_value_t;
typedef time_t sensor_ts_t;         // UTC timestamp as returned by time() - notice that the size of time_t is different on 32/64 bit machine
typedef dplist_node_t sensor_node_t;
typedef dplist_t sensor_list_t;    // UTC timestamp as returned by time() - notice that the size of time_t is different on 32/64 bit machine

typedef struct {
    sensor_id_t id;
    sensor_value_t value;
    sensor_ts_t ts;
} sensor_data_t;

typedef struct {
    char time;
    int id;
    char log[BUFFER_SIZE];
} LogEvent;

extern sem_t *log_sem;


#endif /* _CONFIG_H_ */
