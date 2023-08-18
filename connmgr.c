#define _GNU_SOURCE

/**
 * \author Amine Ayadi
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include "lib/tcpsock.h"
#include "lib/dplist.h"
#include "config.h"
#include "sbuffer.h"
#include "connmgr.h"

#define MAX_CONN 5 
#define MAX_EVENTS 5
#define GW_ON 1

void dpl_print(dplist_t *list);

dplist_t *list_connections;

void *conn_element_copy(void *element)
{
    // check for null pointer
    if (!element)
        return NULL;
    my_epoll_data *sensor_element = (my_epoll_data *)element;
    my_epoll_data *element_copy;
    memcpy(&element_copy, &sensor_element, sizeof(sensor_element));
    return (my_epoll_data *)element_copy;
}

void conn_element_free(void **element)
{
    // pointer check
    if (!(*element))
        return;
    my_epoll_data *element_to_free = *element;
    free(element_to_free);
}

int conn_element_compare(void *x, void *y) {
    if (!x || !y)
        return -1;

    my_epoll_data *sensor_x = (my_epoll_data *)x;
    my_epoll_data *sensor_y = (my_epoll_data *)y;

    if (sensor_x->sensor_id == sensor_y->sensor_id)
        return 0;
    else if (sensor_x->sensor_id > sensor_y->sensor_id)
        return 1;
    else
        return -1;
}

void *safe_malloc(size_t size) {
    void *mem = malloc(size);
    if (mem == NULL) {
        log_msg("Error: Memory allocation failed");
        // We exit because memory allocation failed and further operation can't be continued
        exit(EXIT_FAILURE);
    }
    return mem;
}

void manage_connections(dplist_t* list_connections, my_epoll_data *epoll_server, my_epoll_data *new_client, int fd) 
{
    tcpsock_t *client;
    if (tcp_wait_for_connection(epoll_server->socket_id, &client) != TCP_NO_ERROR) log_msg("Client could not connect to the gateway");
    if (tcp_get_sd(client, &fd) != TCP_NO_ERROR) fprintf(stderr, "[1 - CONNMGR THREAD] Server SD wasn't saved!!\n");
    
    //TODO:
    log_msg("NEW CLIENT %p\n", new_client);
    log_msg("NEW EPOLL DATA %p\n", new_client->epoll_data);
    new_client->epoll_data->fd = fd;
    new_client->socket_id = client;
    new_client->last_modified = time(NULL);

    list_connections = dpl_insert_at_index(list_connections, (void *)(new_client), 0, true);

    fprintf(stderr, "[1 - CONNMGR THREAD] Client added to DPLIST\n");
}

void shutdown_server(tcpsock_t *server) 
{
    tcp_close(&server);
}

void close_client_connection(my_epoll_data *new_client, dplist_t *list_connections, int i) {
    tcp_close(&new_client->socket_id);
    dpl_remove_at_index(list_connections, i, true);
    //dpl_print(list_connections);
    //free(new_client);
}

my_epoll_data* create_server(int port) {
    tcpsock_t *server;
    int server_fd;
    if ((server_fd = tcp_passive_open(&server, port)) != TCP_NO_ERROR) {
        log_msg("Error: Failed to create server");
        exit(EXIT_FAILURE);
    }
    if (tcp_get_sd(server, &server_fd) != TCP_NO_ERROR) {
        log_msg("[1 - CONNMGR THREAD] Server SD was not saved!!\n");
    }
    
    my_epoll_data* epoll_server = safe_malloc(sizeof(my_epoll_data));
    epoll_data_t* epoll_data = safe_malloc(sizeof(epoll_data_t*));
    epoll_server->epoll_data = epoll_data;
    epoll_server->epoll_data->fd = server_fd;
    epoll_server->socket_id = server;
    epoll_server->last_modified = time(NULL);
    
    return epoll_server;
}


void *connmgr_listen(void *argconn)
{
    /****************** Initial Setup **************************/ 
    connmgr_args* args= (connmgr_args*) argconn;
    sbuffer_t* buffer = args->buffer;
    list_connections = dpl_create(conn_element_copy, conn_element_free, conn_element_compare);

    /****************** Create and bind server socket *****************/ 
    my_epoll_data* epoll_server = safe_malloc(sizeof(my_epoll_data*));
    epoll_server = create_server(args->port);
    list_connections = dpl_insert_at_index(list_connections, (void *)(epoll_server), 0, true);
    
    /****************** epoll setup *****************/
    int efd = 0, max_events = 0;
    max_events = MAX_EVENTS;
    if((efd = epoll_create1(0)) == -1){
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
    int listener = epoll_server->epoll_data->fd;
    struct epoll_event ev, ep_event[max_events];
    //ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLOUT | EPOLLMSG | EPOLLWAKEUP | EPOLLRDHUP;
    ev.events =EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP;
    //ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = epoll_server->epoll_data->fd;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, listener, &ev) == -1) {
        perror("epoll_ctl_1");
        exit(EXIT_FAILURE);
    }
    int nfds = 0;

    /****************** [LOG PROCESS] *****************/
    log_msg("[1 - CONNMGR THREAD] Gateway is ON: Accepting connections\n\n");
    //log_event("[1 - CONNMGR THREAD] Server started listening for new sockets\n");
    
    
    while(GW_ON)
    {
        // wait for cond var thread consumer 2 -> producer
        //TODO:
        // while(buffer->head){
        //     log_msg("[1 - CONNMGR THREAD] Thread[%d] %lu waiting on condition variable\n", pthread_self());
        //     pthread_cond_wait(&buffer->buffer_no_data, &buffer->sbuffer_mutex);
        // }
        int num_connections = dpl_size(list_connections);
        // readfds if ready for reading
        //ep_event;
        if ((nfds = epoll_wait(efd, ep_event, max_events, TIMEOUT)) == -1) {
            perror("epoll_wait_2");
            exit(EXIT_FAILURE);
        }

        // ready sockets, check readfds

        //log_msg("NFDS ===== %d\n", nfds);
        for (int i = 0; i < nfds; i++) {
            if (ep_event[i].events == EPOLLIN) {
                //log_msg("EP EVENT [%d] = %b ###############################\n", i, ep_event[i].events);
                // server: new client requesting to connect
                if (ep_event[i].data.fd == listener) {
 
                    my_epoll_data *new_client = safe_malloc(sizeof(my_epoll_data));
                    new_client->epoll_data = safe_malloc(sizeof(epoll_data_t));
                    log_msg("after safe malloc  %p\n", new_client->epoll_data);
                    manage_connections(list_connections, epoll_server, new_client, listener);
                    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP;
                    ev.data = *new_client->epoll_data;
                    ev.data.fd = new_client->epoll_data->fd;
                    ev.data.ptr = new_client; 
                    if (epoll_ctl(efd, EPOLL_CTL_ADD, new_client->epoll_data->fd, &ev) == -1) {
                        perror("epoll_ctl_server");
                        exit(EXIT_FAILURE);
                    }
                    /*TODO:*****LOG********/  
                    log_msg("[1 - CONNMGR THREAD] Number of connections is: %d\n", num_connections);
                    if ((MAX_EVENTS-max_events) == 1) max_events++;//TODO:
                    continue;
                }
                // client: data from existing sensor, read
                else {
                    my_epoll_data* client = ep_event[i].data.ptr;
                    sensor_data_t data; 
                    int bytes, result;
                    bytes = sizeof(data.id);
                    result = tcp_receive(client->socket_id, (void *)&data.id, &bytes);
                    bytes = sizeof(data.value);
                    result = tcp_receive(client->socket_id, (void *)&data.value, &bytes);
                    bytes = sizeof(data.ts);
                    result = tcp_receive(client->socket_id, (void *)&data.ts, &bytes);
                    client->last_modified = time(NULL);
                    //log_msg("sensor id = %hd" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value, (long int)data.ts);     
                    
                    pthread_mutex_lock(&buffer->sbuffer_mutex);
                    sbuffer_insert(buffer, &data);
                    log_msg("\n[1 - CONNMGR THREAD] buffer size is %d", buffer_size(buffer));
                    pthread_cond_signal(&buffer->buffer_has_data1);
                    pthread_mutex_unlock(&buffer->sbuffer_mutex);
                    continue;            
                }
            }  else if (ep_event[i].events & (EPOLLIN | EPOLLRDHUP))
            {
                // ... (handle client failure)
                log_msg("[1 - CONNMGR THREAD] Client failure..");
                my_epoll_data* client = ep_event[i].data.ptr;
                client = dpl_get_element_at_index(list_connections, i);
                close_client_connection(client, list_connections, i);
                epoll_server->last_modified = time(NULL);

                char message[100];
                sprintf(message, "[1 - CONNMGR THREAD] Client of sensor id %d has disconnected due to timeout", client->sensor_id);
                // log_event(message);
            }
        }
        // server: handle server timeouts
        if (epoll_server->last_modified + TIMEOUT < time(NULL) && dpl_size(list_connections) == 1)
        {
            log_msg("[1 - CONNMGR THREAD] No data to be received!! Gateway is shutting down \n");
            if (tcp_close(&epoll_server->socket_id) != TCP_NO_ERROR){
                // -------------[LOG PROCESS]-------------------
                char message[100];
                sprintf(message, "[1 - CONNMGR THREAD] Shutting down!!!\n");
                //log_event(message);
                // -------------[LOG PROCESS]-------------------
                log_msg("[1 - CONNMGR THREAD] Test server is shutting down \n");
                free(list_connections);
                free(epoll_server->epoll_data);
                free(epoll_server);
                buffer->exit_flag = 1;
                sbuffer_free(&buffer);
                pthread_exit(NULL);
                return NULL;
            } else {
                perror("tcp_close");
                exit(EXIT_FAILURE);
            } 
        }
    }
    close(efd);
    exit(EXIT_SUCCESS);
}