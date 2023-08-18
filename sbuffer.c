/**
 * \author Amine Ayadi
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include "config.h"
#include "sbuffer.h"

/**
 * Allocates and initializes a new shared buffer
 * \param buffer a double pointer to the buffer that needs to be initialized
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */

void log_msg(const char* format, ...) {
    va_list args;
    va_start(args, format);
    //printf("[%s]: ", ctime(&now));
    vprintf(format, args);
    va_end(args);
    printf("(Thread %ld)\n", pthread_self());
    fflush(stdout);
}

int sbuffer_init(sbuffer_t **buffer) {
    *buffer = malloc(sizeof(sbuffer_t));
    if (*buffer == NULL) return SBUFFER_FAILURE;
    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;
    (*buffer)->exit_flag = 0;
    pthread_mutex_init(&(*buffer)->sbuffer_mutex, NULL);
    pthread_mutex_init(&(*buffer)->fifo_mutex, NULL);
    pthread_cond_init(&(*buffer)->buffer_no_data, NULL);
    pthread_cond_init(&(*buffer)->buffer_has_data1, NULL);
    pthread_cond_init(&(*buffer)->buffer_has_data2, NULL);
    printf("BUFFER INITIALIZED \n");
    return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **buffer) {
    sbuffer_node_t *dummy; 
    if ((buffer == NULL) || (*buffer == NULL)) {
        return SBUFFER_FAILURE;
    }
    while ((*buffer)->head) {
        dummy = (*buffer)->head;
        (*buffer)->head = (*buffer)->head->next;
        free(dummy);
    }
    free(*buffer);
    *buffer = NULL;
    return SBUFFER_SUCCESS;
}

sensor_data_t* sbuffer_get_first(sbuffer_t *buffer) {
    if(!buffer) return NULL;
	sbuffer_node_t* buf = buffer->head;
    if(!buf) return NULL;
    fprintf(stderr, "[SBUFFER] FIRST VALUE \n");
	printf("DATA RETRIEVED IS %d \n", (buf->data.id));
    return &(buf->data);
}

//
sensor_data_t* sbuffer_get_next(sbuffer_t* buffer)
{
    // Lock the mutex
    pthread_mutex_lock(&buffer->sbuffer_mutex);

    // Check if the buffer is empty
    if (buffer_size(buffer) == 0) {
        // Unlock the mutex and return NULL
        pthread_mutex_unlock(&buffer->sbuffer_mutex);
        return NULL;
    }

    // Save the current node
    sbuffer_node_t* current_node = buffer->head;

    // Update the head to the next node
    buffer->head = buffer->head->next;

    // Unlock the mutex
    pthread_mutex_unlock(&buffer->sbuffer_mutex);

    // Allocate memory for the data
    sensor_data_t* data = malloc(sizeof(sensor_data_t));

    // Copy the data from the current node
    *data = current_node->data;

    // Free the memory for the current node
    free(current_node);

    // Return the copied data
    return data;
}
//

int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data) {
    sbuffer_node_t* dummy;

    if (buffer == NULL) return SBUFFER_FAILURE;
    if (buffer->head == NULL) return SBUFFER_NO_DATA;
    *data = buffer->head->data;
    dummy = buffer->head;
    if (buffer->head == buffer->tail) // buffer has only one node
    {
        buffer->head = buffer->tail = NULL;
    } else  // buffer has many nodes empty
    {
        buffer->head = buffer->head->next;
    }
    free(dummy);
    return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data) {
    sbuffer_node_t *dummy;
    //pthread_rwlock_t* locker = &(buffer)->locker;
    if (buffer == NULL) return SBUFFER_FAILURE;
    dummy = malloc(sizeof(sbuffer_node_t));
    if (dummy == NULL) return SBUFFER_FAILURE;
    dummy->data = *data;
    dummy->managed = 0;
    dummy->stored = 0;
    dummy->next = NULL;
    //pthread_rwlock_wrlock(locker);
    //pthread_mutex_lock(&buffer->mutex);
    if (buffer->tail == NULL) // buffer empty (buffer->head should also be NULL
    {
        buffer->head = buffer->tail = dummy;
    } else // buffer not empty
    {
        buffer->tail->next = dummy;
        buffer->tail = buffer->tail->next;
    }
    //pthread_rwlock_unlock(locker);
    //pthread_mutex_unlock(&buffer->mutex);
    return SBUFFER_SUCCESS;
}

int buffer_has_data(sbuffer_t* buffer){
    if(!buffer) return SBUFFER_FAILURE;
    if(!buffer->head) return SBUFFER_NO_DATA;

    return SBUFFER_HAS_DATA;
}

int buffer_size(sbuffer_t* buffer){
    if(!buffer) return 0;
    if(!buffer->head) return 0;
    // pthread_rwlock_t* locker = &(buffer)->locker;
    // pthread_rwlock_wrlock(locker);
    //pthread_mutex_lock(&buffer->mutex);
    sbuffer_node_t* buf = buffer->head;
    int count = 0;
    while(buf != NULL){
        count++;
        buf = buf->next;
    }
    // pthread_rwlock_unlock(locker);
    //pthread_mutex_unlock(&buffer->mutex);
    return count;
}

char* get_time_format(time_t time){
    struct tm *time_struct = localtime(&time);
    char* time_str = malloc(20 * sizeof(char));
    sprintf(time_str, "[%02d:%02d:%02d]: ", time_struct->tm_hour, time_struct->tm_min, time_struct->tm_sec);
    return time_str;
}  

void log_event(const char* event_message)
{
    if (event_message == NULL) {
        log_msg("Error: event_message is NULL\n");
        return;
    } 
    
    // Get the current time
    time_t current_time = time(NULL);
    
    if (current_time == (time_t)-1) {
        perror("Error: time failed");
        exit(EXIT_FAILURE);
    } else log_msg("[LOG] TIME SUCCESS\n");
    if (!current_time) {
        perror("Error: current time failed");
        exit(EXIT_FAILURE);
    } else log_msg("[LOG] CURRENT TIME SUCCESS\n");
    
    struct tm *time_info = localtime(&current_time);

    if (time_info == NULL) {
        perror("Error: localtime failed");
        //exit(EXIT_FAILURE);
    } else log_msg("[LOG] LOCALTIME SUCCESS\n");
    
    char time_str[50];
    if(strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info) == 0) {
        log_msg("Error: strftime failed to format time\n");
        return;
    }
    // Open the FIFO  for writing
    int fd = open(FIFO_NAME, O_WRONLY);
    if (fd < 0) {
        perror("Error opening FIFO for writing");
        exit(EXIT_FAILURE);
    } else {
        log_msg("[LOG] FIFO: WRITING\n");
    } 
    
    // Write the log event message to the FIFO
    int ret = write(fd, time_str, strlen(time_str));
    if(ret < 0) log_msg("************write failure*********** \n");
    else log_msg("************write success*********** \n");

    ret = write(fd, event_message, strlen(event_message));
    if(ret < 0) log_msg("************write failure*********** \n");
    else log_msg("************write success*********** \n");
    
    // Close the FIFO
    close(fd);
    log_msg("[LOG] Event logged\n");
}