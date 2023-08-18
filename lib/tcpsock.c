/**
 * \author Luc Vandeurzen
 */

#define _GNU_SOURCE

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/poll.h>
#include "tcpsock.h"

//#define DEBUG

#ifdef DEBUG
#define TCP_DEBUG_PRINTF(condition,...)									                        \
        do {												                                    \
           if((condition)) 										                                \
           {												                                    \
            fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);	\
            fprintf(stderr,__VA_ARGS__);								                        \
           }												                                    \
        } while(0)
#else
#define TCP_DEBUG_PRINTF(...) (void)0
#endif


#define TCP_ERR_HANDLER(condition, ...)                                         \
    do {                                                                        \
        if ((condition))                                                        \
        {                                                                       \
          TCP_DEBUG_PRINTF(1,"error condition \"" #condition "\" is true\n");   \
          __VA_ARGS__;                                                          \
        }                                                                       \
    } while(0)


#define MAGIC_COOKIE    (long)(0xA2E1CF37D35)   // used to check if a socket is bounded

#define    CHAR_IP_ADDR_LENGTH  16              // 4 numbers of 3 digits, 3 dots and \0
#define    PROTOCOLFAMILY       AF_INET         // internet protocol suite
#define    TYPE                 SOCK_STREAM     // streaming protool type
#define    PROTOCOL             IPPROTO_TCP     // TCP protocol

/**
 * Structure for holding the TCP socket information
 */
struct tcpsock {
    long cookie;        /**< if the socket is bound, cookie should be equal to MAGIC_COOKIE */
    // remark: the use of magic cookies doesn't guarantee a 'bullet proof' test
    int sd;             /**< socket descriptor */
    char *ip_addr;      /**< socket IP address */
    int port;           /**< socket port number */
};

static tcpsock_t *tcp_sock_create();

int tcp_passive_open(tcpsock_t **sock, int port) {
    int result;
    struct sockaddr_in addr;
    TCP_ERR_HANDLER(((port < MIN_PORT) || (port > MAX_PORT)), return TCP_ADDRESS_ERROR);
    tcpsock_t *s = tcp_sock_create();
    TCP_ERR_HANDLER(s == NULL, return TCP_MEMORY_ERROR);
    s->sd = socket(PROTOCOLFAMILY, TYPE, PROTOCOL);
    TCP_DEBUG_PRINTF(s->sd < 0, "Socket() failed with errno = %d [%s]", errno, strerror(errno));
    TCP_ERR_HANDLER(s->sd < 0, free(s);return TCP_SOCKOP_ERROR);
    // Construct the server address structure
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = PROTOCOLFAMILY;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    result = bind(s->sd, (struct sockaddr *) &addr, sizeof(addr));
    TCP_DEBUG_PRINTF(result == -1, "Bind() failed with errno = %d [%s]", errno, strerror(errno));
    TCP_ERR_HANDLER(result != 0, free(s);return TCP_SOCKOP_ERROR);
    result = listen(s->sd, MAX_PENDING);
    TCP_DEBUG_PRINTF(result == -1, "Listen() failed with errno = %d [%s]", errno, strerror(errno));
    TCP_ERR_HANDLER(result != 0, free(s);return TCP_SOCKOP_ERROR);
    s->ip_addr = NULL; // address set to INADDR_ANY - not a specific IP address
    s->port = port;
    s->cookie = MAGIC_COOKIE;
    *sock = s;
    return TCP_NO_ERROR;
}

int tcp_active_open(tcpsock_t **sock, int remote_port, char *remote_ip) {
    struct sockaddr_in addr;
    tcpsock_t *client;
    int length, result;
    char *p;
    TCP_ERR_HANDLER(((remote_port < MIN_PORT) || (remote_port > MAX_PORT)),
                    return TCP_ADDRESS_ERROR);  // server port between 0 and MIN_PORT is allowed
    TCP_ERR_HANDLER(remote_ip == NULL, return TCP_ADDRESS_ERROR);
    client = tcp_sock_create();
    TCP_ERR_HANDLER(client == NULL, return TCP_MEMORY_ERROR);
    client->sd = socket(PROTOCOLFAMILY, TYPE, PROTOCOL);
    TCP_DEBUG_PRINTF(client->sd < 0, "Socket() failed with errno = %d [%s]", errno, strerror(errno));
    TCP_ERR_HANDLER(client->sd < 0, free(client);return TCP_SOCKOP_ERROR);
    /* Construct the server address structure */
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = PROTOCOLFAMILY;
    result = inet_aton(remote_ip, (struct in_addr *) &addr.sin_addr.s_addr);
    TCP_ERR_HANDLER(result == 0, free(client);return TCP_ADDRESS_ERROR);
    addr.sin_port = htons(remote_port);
    result = connect(client->sd, (struct sockaddr *) &addr, sizeof(addr));
    TCP_DEBUG_PRINTF(result == -1, "Connect() failed with errno = %d [%s]", errno, strerror(errno));
    TCP_ERR_HANDLER(result != 0, free(client);return TCP_SOCKOP_ERROR);
    memset(&addr, 0, sizeof(struct sockaddr_in));
    length = sizeof(addr);
    result = getsockname(client->sd, (struct sockaddr *) &addr, (socklen_t *) &length);
    TCP_DEBUG_PRINTF(result == -1, "getsockname() failed with errno = %d [%s]", errno, strerror(errno));
    TCP_ERR_HANDLER(result != 0, free(client);return TCP_SOCKOP_ERROR);
    p = inet_ntoa(addr.sin_addr);  //returns addr to statically allocated buffer
    client->ip_addr = (char *) malloc(sizeof(char) * CHAR_IP_ADDR_LENGTH);
    TCP_ERR_HANDLER(client->ip_addr == NULL, free(client);return TCP_MEMORY_ERROR);
    client->ip_addr = strncpy(client->ip_addr, p, CHAR_IP_ADDR_LENGTH);
    client->port = ntohs(addr.sin_port);
    client->cookie = MAGIC_COOKIE;
    *sock = client;
    return TCP_NO_ERROR;
}

int tcp_close(tcpsock_t **socket) {
    int result;
    if (socket == NULL) return TCP_SOCKET_ERROR;
    if (*socket == NULL) return TCP_SOCKET_ERROR;
    if ((*socket)->cookie == MAGIC_COOKIE) // socket is bound
    {
        if ((*socket)->ip_addr != NULL) // then assume memory is allocated and must be freed
        {
            free((*socket)->ip_addr);
        }
        if ((*socket)->sd >= 0) {
            // maybe a connection is still open?
            result = shutdown((*socket)->sd, SHUT_RDWR);
            //if ((result of shutdown==-1)&&(errno!=ENOTCONN)) //socket wasn't connected
            TCP_DEBUG_PRINTF(result == -1, "Shutdown() failed with errno = %d [%s]", errno, strerror(errno));
            if (result != -1) {
                result = close((*socket)->sd); // try to close the socket descriptor
                TCP_DEBUG_PRINTF(result == -1, "Close() failed with errno = %d [%s]", errno, strerror(errno));
            }
        }
    }
    // overwrite memory before free to make socket invalid (even if memory is accidently reused)!
    (*socket)->cookie = 0;
    (*socket)->port = -1;
    (*socket)->sd = -1;
    (*socket)->ip_addr = NULL;
    free(*socket);
    *socket = NULL;
    return TCP_NO_ERROR;
}

int tcp_wait_for_connection(tcpsock_t *socket, tcpsock_t **new_socket) {
    struct sockaddr_in addr;
    tcpsock_t *s;
    unsigned int length = sizeof(struct sockaddr_in);
    char *p;
    TCP_ERR_HANDLER(socket == NULL, return TCP_SOCKET_ERROR);
    TCP_ERR_HANDLER(socket->cookie != MAGIC_COOKIE, return TCP_SOCKET_ERROR);
    s = tcp_sock_create();
    TCP_ERR_HANDLER(s == NULL, return TCP_MEMORY_ERROR);
   s->sd = accept(socket->sd, (struct sockaddr *) &addr, &length);
    TCP_DEBUG_PRINTF(s->sd == -1, "Accept() failed with errno = %d [%s]", errno, strerror(errno));
    TCP_ERR_HANDLER(s->sd == -1, free(s);return TCP_SOCKOP_ERROR);
    p = inet_ntoa(addr.sin_addr);  //returns addr to statically allocated buffer
    s->ip_addr = (char *) malloc(sizeof(char) * CHAR_IP_ADDR_LENGTH);
    TCP_ERR_HANDLER(s->ip_addr == NULL, free(s);return TCP_MEMORY_ERROR);
    s->ip_addr = strncpy(s->ip_addr, p, CHAR_IP_ADDR_LENGTH);
    s->port = ntohs(addr.sin_port);
    s->cookie = MAGIC_COOKIE;
    *new_socket = s;
    return TCP_NO_ERROR;
}

#define TIMEOUT 5

int tcp_poll_for_connection(tcpsock_t *socket, tcpsock_t **new_socket) {
    struct sockaddr_in addr;
    tcpsock_t *s;
    unsigned int length = sizeof(struct sockaddr_in);
    char *p;
    TCP_ERR_HANDLER(socket == NULL, return TCP_SOCKET_ERROR);
    TCP_ERR_HANDLER(socket->cookie != MAGIC_COOKIE, return TCP_SOCKET_ERROR);
    s = tcp_sock_create();
    TCP_ERR_HANDLER(s == NULL, return TCP_MEMORY_ERROR);
    s->sd = accept(socket->sd, (struct sockaddr *) &addr, &length);
    TCP_DEBUG_PRINTF(s->sd == -1, "Accept() failed with errno = %d [%s]", errno, strerror(errno));
    TCP_ERR_HANDLER(s->sd == -1, free(s);return TCP_SOCKOP_ERROR);
    p = inet_ntoa(addr.sin_addr);  //returns addr to statically allocated buffer
    s->ip_addr = (char *) malloc(sizeof(char) * CHAR_IP_ADDR_LENGTH);
    TCP_ERR_HANDLER(s->ip_addr == NULL, free(s);return TCP_MEMORY_ERROR);
    s->ip_addr = strncpy(s->ip_addr, p, CHAR_IP_ADDR_LENGTH);
    s->port = ntohs(addr.sin_port);
    s->cookie = MAGIC_COOKIE;
    *new_socket = s;
    return TCP_NO_ERROR;
}

int tcp_send(tcpsock_t *socket, void *buffer, int *buf_size) {
    TCP_ERR_HANDLER(socket == NULL, return TCP_SOCKET_ERROR);
    TCP_ERR_HANDLER(socket->cookie != MAGIC_COOKIE, return TCP_SOCKET_ERROR);
    if ((buffer == NULL) || (buf_size == 0)) //nothing to send
    {
        *buf_size = 0;
        return TCP_NO_ERROR;
    }
    // if socket is not connected, a SIGPIPE signal is sent which terminates the program (default behaviour)
    //*buf_size = sendto(socket->sd, (const void*)buffer,*buf_size, 0, NULL, 0);
    // use MSG_NOSIGNAL flag to avoid a signal to be sent
    *buf_size = sendto(socket->sd, (const void *) buffer, *buf_size, MSG_NOSIGNAL, NULL, 0);
    TCP_DEBUG_PRINTF((*buf_size == 0), "Send() : no connection to peer\n");
    TCP_ERR_HANDLER(*buf_size == 0, return TCP_CONNECTION_CLOSED);
    TCP_DEBUG_PRINTF(((*buf_size < 0) && ((errno == EPIPE) || (errno == ENOTCONN))),
                     "Send() : no connection to peer\n");
    TCP_ERR_HANDLER(((*buf_size < 0) && ((errno == EPIPE) || (errno == ENOTCONN))), return TCP_CONNECTION_CLOSED);
    TCP_DEBUG_PRINTF(*buf_size < 0, "Send() failed with errno = %d [%s]", errno, strerror(errno));
    TCP_ERR_HANDLER(*buf_size < 0, return TCP_SOCKOP_ERROR);
    return TCP_NO_ERROR;
}

int tcp_receive(tcpsock_t *socket, void *buffer, int *buf_size) {
    TCP_ERR_HANDLER(socket == NULL, return TCP_SOCKET_ERROR);
    TCP_ERR_HANDLER(socket->cookie != MAGIC_COOKIE, return TCP_SOCKET_ERROR);
    if ((buffer == NULL) || (buf_size == 0))  //nothing to read
    {
        *buf_size = 0;
        return TCP_NO_ERROR;
    }
    *buf_size = recv(socket->sd, buffer, *buf_size, 0);
    TCP_DEBUG_PRINTF(*buf_size == 0, "Recv() : no connection to peer\n");
    TCP_ERR_HANDLER(*buf_size == 0, return TCP_CONNECTION_CLOSED);
    TCP_DEBUG_PRINTF((*buf_size < 0) && (errno == ENOTCONN), "Recv() : no connection to peer\n");
    TCP_ERR_HANDLER((*buf_size < 0) && (errno == ENOTCONN), return TCP_CONNECTION_CLOSED);
    TCP_DEBUG_PRINTF(*buf_size < 0, "Recv() failed with errno = %d [%s]", errno, strerror(errno));
    TCP_ERR_HANDLER(*buf_size < 0, return TCP_SOCKOP_ERROR);
    return TCP_NO_ERROR;
}

int tcp_get_ip_addr(tcpsock_t *socket, char **ip_addr) {
    TCP_ERR_HANDLER(socket == NULL, return TCP_SOCKET_ERROR);
    TCP_ERR_HANDLER(socket->cookie != MAGIC_COOKIE, return TCP_SOCKET_ERROR);
    *ip_addr = socket->ip_addr;
    return TCP_NO_ERROR;
}

int tcp_get_port(tcpsock_t *socket, int *port) {
    TCP_ERR_HANDLER(socket == NULL, return TCP_SOCKET_ERROR);
    TCP_ERR_HANDLER(socket->cookie != MAGIC_COOKIE, return TCP_SOCKET_ERROR);
    *port = socket->port;
    return TCP_NO_ERROR;
}

int tcp_get_sd(tcpsock_t *socket, int *sd) {
    TCP_ERR_HANDLER(socket == NULL, return TCP_SOCKET_ERROR);
    TCP_ERR_HANDLER(socket->cookie != MAGIC_COOKIE, return TCP_SOCKET_ERROR);
    *sd = socket->sd;
    return TCP_NO_ERROR;
}

static tcpsock_t *tcp_sock_create() {
    tcpsock_t *s = (tcpsock_t *) malloc(sizeof(tcpsock_t));
    if (s) // init the socket to default values
    {
        s->cookie = 0;  // socket is not yet bound!
        s->port = -1;
        s->ip_addr = NULL;
        s->sd = -1;
    }
    return s;
}
