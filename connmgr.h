/**
 * \author Amine Ayadi
 */

#ifndef _CONNMGR_H_
#define _CONNMGR_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#include "config.h"

#ifndef TIMEOUT
#define TIMEOUT 
#error TIMEOUT not set
#endif

typedef struct {
    epoll_data_t* epoll_data;  
    sensor_id_t sensor_id;
    tcpsock_t* socket_id;
    time_t last_modified;
} my_epoll_data;

typedef struct {
    sbuffer_t* buffer;
    int port;
    int pipe;
} connmgr_args;

typedef struct {
    tcpsock_t *server;
    epoll_data_t *epoll_server;
    sensor_data_t *data;
} ServerPollmanager;

void* connmgr_listen(void* ptr);
/*This method holds the core functionality of your connmgr. 
It starts listening on the given port and when when a sensor node connects 
it writes the data to a sensor_data_recv file. 
This file must have the same format as the sensor_data file in assignment 6 and 7*/

void connmgr_free();
/*This method should be called to clean up the connmgr, 
and to free all used memory. After this no new connections will be accepted*/




#endif /* _CONNMGR_H_ */
