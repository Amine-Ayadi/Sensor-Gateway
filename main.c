#include "config.h"
#include "sensor_db.h"
#include "sbuffer.h"
#include "connmgr.h"
#include "datamgr.h"
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>

static int CHILD_PROCESS_SHOULD_EXIT= 0;

/*void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        //printf("Sent SIGUSR1 Signal\n");
        CHILD_PROCESS_SHOULD_EXIT = 1;
    }
    signal(sig, signal_handler);
}*/


void *start_storemgr(void *arg)
{
    stormgr_args *args = ((stormgr_args *)arg);
    DBCONN *conn = (DBCONN *)args->conn;
    sbuffer_t *buffer = args->buffer;
    if (!conn) {
        log_msg("[3 - STOREMGR THREAD] Error: not connected");
        char message[100];
        sprintf(message, "[3-STOREMGR]: Connected to SQL database.\n");
        log_event(message); 
    }
    else {
        log_msg("[3 - STOREMGR THREAD] CONNECTED");
        char message[100];
        sprintf(message, "[3-STOREMGR]: Unable to connect to SQL database\n");
        log_event(message);
    }

    while (conn && !buffer->exit_flag)
    {
        int ret = pthread_mutex_lock(&buffer->sbuffer_mutex);            
        if (ret != 0)
            log_msg("[3 - STOREMGR THREAD] Error locking mutex: %s", strerror(ret)); // Error handling
        else log_msg("[3 - STOREMGR THREAD] LOCKING..........");
        
       while (!buffer->head)
        {
            log_msg("[3 - STOREMGR THREAD] Thread[%d] %lu waiting on condition variable", 3, pthread_self());
            pthread_cond_wait(&buffer->buffer_has_data2, &buffer->sbuffer_mutex);
            if (buffer->exit_flag) break;
        }
        if (buffer->exit_flag) continue;

        //sbuffer_node_t* node = buffer->head;
        sensor_data_t *data = sbuffer_get_first(buffer);
        int result = insert_sensor(conn, data->id, data->value, data->ts);
        if (result != 0) perror("[3 - STOREMGR THREAD] Error inserting sensor data into database");
        log_msg("[3 - STOREMGR THREAD] SENSOR ID %d: INSERTED VALUE: %.2f AT %ld", data->id, data->value, time(NULL));
        char message[100];
        sprintf(message, "[3-STOREMGR]: New table %s created\n", TO_STRING(TABLE_NAME));
        log_event(message);
        
        sbuffer_remove(buffer, data); // delete data from buffer, makes it a circular buffer
        pthread_mutex_unlock(&buffer->sbuffer_mutex); 
    }
    disconnect(conn);
    log_msg("[3 - STOREMGR THREAD] Disconnected");
    char message[100];
    sprintf(message, "[3-STOREMGR]: Disconnected from database.\n");
    log_event(message);

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

    //signal(SIGUSR1, signal_handler);

    // we try to rewind the pipe if it's blocked or broken 
    //signal(SIGPIPE,SIG_IGN);

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
        printf("Creating new fifo %s\n", FIFO_NAME);
        int result = mkfifo(FIFO_NAME, 0666);
        if (result < 0)
        {
            perror("Error creating FIFO");
            exit(EXIT_FAILURE);
        }
    }

    // PIPING:
    pid_t my_pid;
    pid_t child_pid;
    my_pid = getpid();
    printf("Parent process (pid = %d) is started \n", my_pid);
//#if 0

    child_pid = fork();
    // Check if the fork was successful
    if (child_pid == -1)
    {
        printf("[DID NOT CREATE CHILD PROCESS] \n");
        perror("Error: fork failed");
        free(buffer);
        free(args);
        free(arg);
        free(argcon);
        sqlite3_close(conn);
        exit(EXIT_FAILURE);
    }
    // If we are in the child process (the log process)
    if (child_pid == 0)
    {
        // Open the FIFO for reading
        
        //sleep(1);
        int fd = open(FIFO_NAME, O_RDONLY);
        if (fd < 0)
        {
            perror("Error opening FIFO for reading\n");
            exit(EXIT_FAILURE);
        } else log_msg("File opened: child process\n");

        while(!CHILD_PROCESS_SHOULD_EXIT)
        {

            
            FILE *log_file = fopen("gateway.log", "a+");
            if (log_file == NULL)
            {
                perror("Error opening log file");
                exit(EXIT_FAILURE);
            } //else fprintf(stderr, "fd no data : %d \n", fd);
            
            // Read log events from the FIFO and write them to the log file
            char buff[200];
            int bytes_read;
            int event_count = 0;
            
            while ((bytes_read = read(fd, buff, sizeof(buff) - 1)) > 0)
            {
                // Null-terminate the buffer
                buff[bytes_read] = '\0';
                //log_msg("%s \n", buff);
                fprintf(log_file, "%d %s", ++event_count, buff);

                //log_event

                char* res = strstr(buff, "send_kill_child"); 
                if (res != NULL)                     // if successful then s now points at "hassasin"
                {
                    CHILD_PROCESS_SHOULD_EXIT = 1;
                }

            }
            if(errno == EPIPE){
                log_msg("PIPE ERROR\n");
            }
            fclose(log_file);
            //close(fd);
            
        }
        close(fd);
        exit(EXIT_SUCCESS);
    }
    /*   MAIN PROCESS - 3 threads  */
    else if (my_pid > 0)
    {

        //return 0;
//#endif
        //printf("Parent process (pid = %d) has created child process (pid = %d)...\n", my_pid, child_pid);
        
        //log_sem = sem_open("log_sem", O_CREAT, 0644, 1);

        if (argc < 2) {
            printf("Please provide a port for the server as a command-line argument\n");
            exit(EXIT_FAILURE);
        }

        int port_input = atoi(argv[1]);
        int ret = sbuffer_init(&buffer);
        if(ret != SBUFFER_SUCCESS){
            printf("Failure \n");
            exit(-1);
        }
        args->buffer = buffer;
        arg->buffer = buffer;
        arg->conn = conn;
        argcon->buffer = buffer;
        argcon->port = port_input;
        if (pthread_create(&connmgrID, NULL, start_connmgr, argcon) != 0)
            perror("Failed to create thread connmgrID \n");

        if (pthread_create(&datamgrID, NULL, start_datamgr, args) != 0)
            perror("Failed to create thread datamgrID \n");

        if (pthread_create(&stormgrID, NULL, start_storemgr, arg) != 0)
            perror("Failed to create thread datamgrID \n");

        pthread_join(connmgrID, NULL);
        printf("Thread 1 finished execution... \n");
        pthread_join(datamgrID, NULL);
        printf("Thread 2 finished execution... \n");
        pthread_join(stormgrID, NULL);
        printf("Thread 3 finished execution... \n");

        pthread_mutex_destroy(&buffer->sbuffer_mutex);
        pthread_mutex_destroy(&buffer->fifo_mutex);
        pthread_cond_destroy(&buffer->buffer_has_data1);
        pthread_cond_destroy(&buffer->buffer_has_data2);        
        pthread_cond_destroy(&buffer->buffer_no_data);
        sbuffer_free(&buffer);
        free(args);
        free(arg);
        free(argcon);
        
        // wait on termination of child process
       // log_msg("STUCK main\n");

        char message[100];
        sprintf(message, "Sending SIGUSR1 signal to child process\n");
        log_event(message);

        log_msg("Parent process (pid = %d) is terminating ...\n", my_pid);
        
        //kill(0, SIGUSR1);

        log_event("send_kill_child");

        // wait for child to die
        waitpid(child_pid, NULL, 0);
        
        //kill(child_pid, SIGKILL);
    }


    log_msg("Exit success\n");
    return 0;
}

