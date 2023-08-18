#include "config.h"
#include "sensor_db.h"
#include "sbuffer.h"
#include "connmgr.h"
#include "datamgr.h"
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>


sem_t *log_sem;

void *start_storemgr(void *arg)
{
    stormgr_args *args = ((stormgr_args *)arg);
    DBCONN *conn = (DBCONN *)args->conn;
    sbuffer_t *buffer = args->buffer;
    if (!conn)
        log_msg("[3 - STOREMGR THREAD] Error: not connected");
    else
        log_msg("[3 - STOREMGR THREAD] Connected");

    while (conn && !buffer->exit_flag)
    {
        int ret = pthread_mutex_lock(&buffer->sbuffer_mutex);
        if (ret != 0) { 
            log_msg("[3 - STOREMGR THREAD] Error locking mutex: %s", strerror(ret)); // Error handling
        } else log_msg("[3 - STOREMGR THREAD] LOCKED HERE");

        fprintf(stderr, "[3 - STOREMGR THREAD] STARTED");
        while (!buffer->head)
        {
            log_msg("[3 - STOREMGR THREAD] WAITING: ALLOW WRITING");
            log_msg("[3 - STOREMGR THREAD] Thread[%d] %lu waiting on condition variable", 3, pthread_self());
            pthread_cond_wait(&buffer->buffer_has_data2, &buffer->sbuffer_mutex);
        }
        /****************** STORE DATA IN DB *****************/
        log_msg("[3 - STOREMGR THREAD] Thread %lu finished waiting", pthread_self());
        log_msg("[3 - STOREMGR THREAD] SIZE OF BUFFER IS %d", buffer_size(buffer));

        sensor_data_t *data = sbuffer_get_first(buffer);

        //TODO: [LOG] 
        //run_child(buffer, conn, args->pipe, 0);
        //while (data)
        //{
        log_msg("[3 - STOREMGR THREAD] Reading");
        log_msg("[3 - STOREMGR THREAD] DATA RECEIVING: %.2f", data->value);
        int result = insert_sensor(conn, data->id, data->value, data->ts);
        log_msg("[3 - STOREMGR THREAD] Data Inserted in DataBase");
        if (result != 0) perror("[3 - STOREMGR THREAD] Error inserting sensor data into database");

        /****************** DELETE DATA FROM *****************/
        log_msg("[3 - STOREMGR THREAD] About to remove");
        sbuffer_remove(buffer, data);
        log_msg("[3 - STOREMGR THREAD] Data Removed");

        pthread_cond_signal(&buffer->buffer_no_data);
        log_msg("[3 - STOREMGR THREAD] Signalling condition variable from thread %lu", pthread_self());
        pthread_mutex_unlock(&buffer->sbuffer_mutex);
    }

    // Disconnect from the database when we're done
    disconnect(conn);
    fprintf(stderr, "[3 - STOREMGR THREAD] Disconnected");

    return NULL;
}

void *start_connmgr(void *arg){
    connmgr_listen(arg);
    return NULL;
}

void *start_datamgr(void *arg){
    datamgr_parse_sensor_files(arg);
    return NULL;
}

int main(int argc, char **argv)
{
    // VARIABLES:
    sbuffer_t *buffer;
    DBCONN *conn = init_connection(1);

    // we try to rewind the pipe if it's blocked or broken 
    signal(SIGPIPE,SIG_IGN);

    pthread_t connmgrID;
    pthread_t datamgrID;
    pthread_t stormgrID;
    datamgr_args *args = (datamgr_args *)malloc(sizeof(datamgr_args));
    stormgr_args *arg = (stormgr_args *)malloc(sizeof(stormgr_args));
    connmgr_args *argcon = (connmgr_args *)malloc(sizeof(connmgr_args));
    args->file = fopen("room_sensor.map", "rb");

    // Create the FIFO if it doesn't already exist
    if (access(FIFO_NAME, F_OK) == -1)
    {
        // FIFO does not exist, create it
        int result = mkfifo(FIFO_NAME, 0666);
        if (result < 0)
        {
            perror("Error creating FIFO");
            exit(EXIT_FAILURE);
        }
    }

    // PIPING:
    pid_t my_pid;
    my_pid = getpid();
    printf("child process (pid = %d) is started \n", my_pid);


    // If we are in the child process (the log process)
    if (1 == 1)
    {
        // Open the FIFO for reading
        sleep(1);
        int fd = open(FIFO_NAME, O_RDONLY);
        if (fd < 0)
        {
            perror("Error opening FIFO for reading\n");
            exit(EXIT_FAILURE);
        } else printf("File opened: child process\n");

        while(!buffer->exit_flag){
        // while(1){
            // Open the log file for writing
            FILE *log_file = fopen("gateway.log", "a");
            if (log_file == NULL)
            {
                perror("Error opening log file");
                exit(EXIT_FAILURE);
            } else fprintf(stderr, "fd no data : %d \n", fd);
            
            // Read log events from the FIFO and write them to the log file
            char buff[100];
            int bytes_read;
            int event_count = 0;
            
            while ((bytes_read = read(fd, buff, sizeof(buff) - 1)) > 0)
            {
                // Null-terminate the buffer
                log_msg("odkhel \n");
                buff[bytes_read] = '\0';
                fprintf(log_file, "%d %s", ++event_count, buff);
            }
            if(errno == EPIPE){
                log_msg("PIPE ERROR\n");
            }
            log_msg("odkhel \n");
            fclose(log_file);
            close(fd);
        }
        exit(EXIT_SUCCESS);
    }

    printf("Gateway worked.... AYA SAYEB ZEBI\n");
    return 0;
}

