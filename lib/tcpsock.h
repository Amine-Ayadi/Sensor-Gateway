/**
 * \author Luc Vandeurzen
 */

#ifndef __TCPSOCK_H__
#define __TCPSOCK_H__

#define MIN_PORT    1024
#define MAX_PORT    65536

#define    TCP_NO_ERROR             0
#define    TCP_SOCKET_ERROR         1   // invalid socket
#define    TCP_ADDRESS_ERROR        2   // invalid port and/or IP address
#define    TCP_SOCKOP_ERROR         3   // socket operator (socket, listen, bind, accept,...) error
#define    TCP_CONNECTION_CLOSED    4   // send/receive indicate connection is closed
#define    TCP_MEMORY_ERROR         5   // mem alloc error

#define MAX_PENDING 10

typedef struct tcpsock tcpsock_t;

/**
 * Creates a new socket and opens this socket in 'passive listening mode' (waiting for an active connection setup request)
 * The socket is bound to port number 'port' and to any active IP interface of the system
 * The number of pending connection setup requests is set to MAX_PENDING
 * This function is typically called by a server
 * If port 'port' is not between MIN_PORT and MAX_PORT, TCP_ADDRESS_ERROR is returned
 * If memory allocation for the newly created socket fails, TCP_MEMORY_ERROR is returned
 * If a socket operation (socket, listen, bind, accept,...) fails, TCP_SOCKOP_ERROR is returned
 * \param socket a double pointer, that will be filled out with the newly created socket
 * \param port a port number between MIN_PORT and MAX_PORT
 * \return TCP_NO_ERROR if no error occurs during execution
 */
int tcp_passive_open(tcpsock_t **socket, int port);

/**
 * Creates a new TCP socket and opens a TCP connection to the system with IP address 'remote_ip' on port 'remote_port'
 * The newly created socket is return as '*socket'
 * This function is typically called by a client
 * If port 'remote_port' is not between MIN_PORT and MAX_PORT, TCP_ADDRESS_ERROR is returned
 * If 'remote_ip' is NULL or an IP address operation (inet_aton, ...) fails, TCP_ADDRESS_ERROR is returned
 * If memory allocation for the newly created socket fails, TCP_MEMORY_ERROR is returned
 * If a socket operation (socket, listen, bind, accept,...) fails, TCP_SOCKOP_ERROR is returned
 * \param socket a double pointer, that will be filled out with the newly created socket
 * \param remote_port the remote port number to connect to
 * \param remote_ip the remote ip address to connect to
 * \return TCP_NO_ERROR if no error occurs during execution
 */
int tcp_active_open(tcpsock_t **socket, int remote_port, char *remote_ip);


/**
 * The socket '*socket' is closed , allocated resources are freed and '*socket' is set to NULL
 * If '*socket' is connected, a TCP shutdown on the connection is executed
 * If 'socket' or '*socket' is NULL, nothing is done and TCP_SOCKET_ERROR is returned
 * If '*socket' is not a valid socket, the result of the function is undefined
 * \param socket a double pointer, to the socket that needs to be closed
 * \return TCP_NO_ERROR if no error occurs during execution
 */
int tcp_close(tcpsock_t **socket);

/**
 * Puts the socket 'socket' in a blocking wait mode
 * Returns when an incoming TCP connection setup request is received
 * A newly created socket identifying the remote system that initiated the connection request is returned as '*new_socket'
 * If memory allocation for the new socket fails, TCP_MEMORY_ERROR is returned
 * If a socket operation (socket, listen, bind, accept, ...) fails, TCP_SOCKOP_ERROR is returned
 * If 'socket' is NULL or not yet bound, TCP_SOCKET_ERROR is returned
 * \param socket the socket that needs to be monitored for a new incomming connection
 * \param new_socket a double pointer, that will be filled out with the newly created socket for the connection with the client
 * \return TCP_NO_ERROR if no error occurs during execution
 */
int tcp_wait_for_connection(tcpsock_t *socket, tcpsock_t **new_socket);

int tcp_poll_for_connection(tcpsock_t *socket, tcpsock_t **new_socket);
/**
 * Initiates a send command on the socket 'socket' and tries to send the total '*buf_size' bytes of data in 'buffer' (recall that the function might block for a while)
 * The function sets '*buf_size' to the number of bytes that were really sent, which might be less than the initial '*buf_size'
 * If a socket error happens while sending the data in 'buffer' or the connection is closed, TCP_SOCKOP_ERROR or TCP_CONNECTION_CLOSED is returned, respectively
 * If 'socket' is NULL or not yet bound, TCP_SOCKET_ERROR is returned
 * \param socket the socket where the data needs to be sent on
 * \param buffer a pointer to the buffer that holds the data that needs to be sent
 * \param buf_size the amount of bytes that need to be sent from the buffer
 * \return TCP_NO_ERROR if no error occurs during execution
 */
int tcp_send(tcpsock_t *socket, void *buffer, int *buf_size);

/**
 * Initiates a receive command on the socket 'socket' and tries to receive the total '*buf_size' bytes of data in 'buffer' (recall that the function might block for a while)
 * The function sets '*buf_size' to the number of bytes that were really received, which might be less than the inital '*buf_size'
 * If a socket error happens while receiving data or the connection is closed, TCP_SOCKOP_ERROR or TCP_CONNECTION_CLOSED is returned, respectively
 * If 'socket' is NULL or not yet bound, TCP_SOCKET_ERROR is returned
 * \param socket the socket where the data needs to be received from
 * \param buffer a pointer to the buffer that can store the data that is received
 * \param buf_size the amount of bytes that will be read from the socket
 * \return TCP_NO_ERROR if no error occurs during execution
 */
int tcp_receive(tcpsock_t *socket, void *buffer, int *buf_size);

/**
 * Set '*ip_addr' to the IP address of 'socket' (could be NULL if the IP address is not set)
 * No memory allocation is done (pointer reference assignment!), hence, no free must be called to avoid a memory leak
 * If 'socket' is NULL or not yet bound, TCP_SOCKET_ERROR is returned
 * \param socket the socket to get the ip address from
 * \param ip_addr a pointer to a char* that can hold the ip address
 * \return TCP_NO_ERROR if no error occurs during execution
 */
int tcp_get_ip_addr(tcpsock_t *socket, char **ip_addr);

/**
 * Return the port number of the 'socket'
 * If 'socket' is NULL or not yet bound, TCP_SOCKET_ERROR is returned
 * \param socket the socket to get the port number from
 * \param port a pointer to an int that can hold the port number
 * \return TCP_NO_ERROR if no error occurs during execution
 */
int tcp_get_port(tcpsock_t *socket, int *port);

/**
 * Return the socket descriptor of the 'socket'
 * If 'socket' is NULL or not yet bound, TCP_SOCKET_ERROR is returned
 * \param socket the socket to get the socket descriptor from
 * \param port a pointer to an int that can hold the socket descriptor
 * \return TCP_NO_ERROR if no error occurs during execution
 */
int tcp_get_sd(tcpsock_t *socket, int *sd);

#endif  //__TCPSOCK_H__
