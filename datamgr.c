#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>

#include "config.h"
#include "datamgr.h"

#define DPLIST_NO_ERROR 0
#define DPLIST_MEMORY_ERROR 1  // error due to mem alloc failure
#define DPLIST_INVALID_ERROR 2 // error due to a list operation applied on a NULL list

#ifdef DEBUG
#define DEBUG_PRINTF(...)                                                                    \
    do                                                                                       \
    {                                                                                        \
        fprintf(stderr, "\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                                                        \
        fflush(stderr);                                                                      \
    } while (0)
#else
#define DEBUG_PRINTF(...) (void)0
#endif

#define BUFFER_SIZE 264
#define WAIT_TIME_SECONDS 15

#define DPLIST_ERR_HANDLER(condition, err_code)   \
    do                                            \
    {                                             \
        if ((condition))                          \
            DEBUG_PRINTF(#condition " failed\n"); \
        assert(!(condition));                     \
    } while (0)

typedef struct
{
    sensor_id_t sensor_id;
    int room_id;
    double running_avg;
    double temp[RUN_AVG_LENGTH];
    int temp_index; // TODO:
    time_t last_modified;
} sensor_t;

void *element_copy(void *element);
void element_free(void **element);
int element_compare(void *x, void *y);

void function(sensor_t *tdata);
sensor_list_t *read_sensor_data(char *filename);
sensor_list_t *read_sensor_map(char *filename);
void dpl_free(dplist_t **list, bool free_element);
void sensor_print_updated(sbuffer_t* buffer, sensor_list_t *list, double value);
sensor_value_t datamgr_compute_avg(sensor_t *sensor_data, sensor_id_t sensor_id);
void dpl_print(dplist_t *list);
void sensor_list_print(sensor_list_t *sensor_list);

sensor_list_t *sensor_list;

void* datamgr_parse_sensor_files(void* ptr)
{
    datamgr_args* args = ((datamgr_args*)ptr);
    sensor_list = dpl_create(element_copy, element_free, element_compare);
    FILE *fp_data_log = fopen("sensor_data_log.txt", "w");
    int buffer_room;
    sbuffer_t* buffer = args->buffer;
    uint16_t buffer_sensor;
    sensor_t *sensor;

    if (args->buffer == NULL) {
        log_msg("[2 - DATAMGR THREAD] Error: buffer is null");
    }

    /****************** GET ROOM AND SENSOR MAP *****************/
    while (fscanf(args->file, "%d %hd", &buffer_room, &buffer_sensor) > 0)
    {
        sensor = malloc(sizeof(sensor_t));
        sensor->room_id = (int)buffer_room;
        sensor->sensor_id = (int)buffer_sensor;
        for (int i = 0; i < RUN_AVG_LENGTH; i++)
        {
            sensor->temp[i] = 0;
        }
        sensor->running_avg = 0;
        sensor->last_modified = 0;
        dpl_insert_sorted(sensor_list, sensor, true);
    }
    log_msg("[2 - DATAMGR THREAD] SENSOR MAP INSERTED");
    fclose(args->file);
    /****************** [LOG PROCESS] *****************/
    char message[100];
    // log_event()
    /****************** TIME AND TEMP INTO DPLIST *****************/
    int j = 0;
    while(!buffer->exit_flag)
    {
        sbuffer_node_t* node = buffer->head;
        if (!node) {
            //log_msg("[2 - DATAMGR THREAD] Waiting: head of buffer is null");
            //continue;
        }
        // log_msg("[DATAMGR]TRYING TO LOCK... %d  \n", pthread_mutex_trylock(&buffer->sbuffer_mutex));
        // int ret = pthread_mutex_lock(&buffer->sbuffer_mutex);
        // log_msg("Error locking mutex: %s\n", strerror(ret)); 
        
        while(!buffer->head){
            log_msg("[2 - DATAMGR THREAD] Thread[%d] %lu waiting on condition variable", 2, pthread_self());
            pthread_cond_wait(&buffer->buffer_has_data1, &buffer->sbuffer_mutex);
        }
        log_msg("[2 - DATAMGR THREAD] Thread %lu finished waiting", pthread_self());
        // log_msg("[2 - DATAMGR THREAD] SIZE OF BUFFER IS %d", buffer_size(buffer));

        log_msg("[2 - DATAMGR THREAD] buffer received from sensor_id: %d ", buffer->head->data.id);
        //int res = pthread_mutex_lock(&buffer->sbuffer_mutex);
        //log_msg("[2 - DATAMGR THREAD] mutex lock %d \n", res);
        sensor_data_t* data = sbuffer_get_first(buffer); 
        // -----------Find the sensor in the list---------------
        for (int i = 0; i < dpl_size(sensor_list); i++)
        {
            sensor_t *sensor = dpl_get_element_at_index(sensor_list, i);
            if (sensor->sensor_id == data->id)
            {
                // Add the value from buffer_temp to the temp array at index j
                node = buffer->head;
                int i;
                double deletedValue = sensor->temp[0];
                log_msg("Room ID : %d ", sensor->room_id);
                log_msg("Sensor ID : %hd ", sensor->sensor_id);
                for (i = 0; i < RUN_AVG_LENGTH; i++) {
                    sensor->temp[i] = sensor->temp[i+1];  
                }
                sensor->temp[RUN_AVG_LENGTH-1] = data->value;
                sensor->last_modified = data->ts;
                time_t t = sensor->last_modified;
                struct tm *tm = localtime(&t);
                if(tm){
                    char* time_str = malloc(20 * sizeof(char));
                    sprintf(time_str, "[%02d:%02d:%02d]: ", tm->tm_hour, tm->tm_min, tm->tm_sec);
                    log_msg("Last modified is at : %s ", time_str);
                }
                sensor->running_avg = sensor->running_avg - (deletedValue/RUN_AVG_LENGTH) + (sensor->temp[RUN_AVG_LENGTH-1]/RUN_AVG_LENGTH);
                for(int i=0;i<RUN_AVG_LENGTH;i++)
                    log_msg("\tValue_recoded : %.2f ", sensor->temp[i]);
                log_msg("Running Average : %.2f ", sensor->running_avg); 
                // -------------[LOG PROCESS]-------------------
                char message[100];
                sprintf(message, "[2 - DatamgrID]: In room %d, a new running average recorded for sensor %d", sensor->room_id, sensor->sensor_id);
                //log_event(message);
                // -------------[LOG PROCESS]-------------------
                if(!sensor_list){
                    log_msg("NULL LIST");
                    return NULL;
                }
                dplist_node_t* tmp = sensor_list->head;
                log_msg("[2 - DatamgrID]: BUFFER SIZE NOW IS %d\n", buffer_size(buffer));
                if (!tmp) {
                    log_msg("Head pointing to NULL");
                    return NULL;
                }         
            }            
            j++;
            if (j == RUN_AVG_LENGTH)
                j = 0;    
        }

        //sbuffer_remove(buffer, data);
        pthread_cond_signal(&buffer->buffer_has_data2);
        pthread_mutex_unlock(&buffer->sbuffer_mutex);
        
        fprintf(fp_data_log,
                "<Room: %u>\t<Sensor: %u>\t<AvgTemp: %lf>\t\t<Time: %ld>\n",
                sensor->room_id,
                sensor->sensor_id,
                sensor->running_avg,
                sensor->last_modified);

        /*TODO:***************** [LOG][GATEWAY MESSAGE] *****************/                
        if (sensor->running_avg < SET_MIN_TEMP){
            log_msg("\n\t[LOG][2 - DATAMGR THREAD] Temperature dropped in room %hu. \n\tThe average temperature recorded by sensor %d is %.2f at %ld\n\n", sensor->room_id, sensor->sensor_id, sensor->running_avg, sensor->last_modified);
            char message[100];
            sprintf(message, "Temperature dropped in room %d. \n\tThe average temperature recorded by sensor %d is %.2f\n\n", sensor->room_id, sensor->sensor_id, sensor->running_avg);
            //log_event(message);
        }   
        else if (sensor->running_avg > SET_MAX_TEMP){
            fprintf(stderr, "\n\t[2 - DATAMGR THREAD] Temperature increased in room %hu. \n\tThe average temperature recorded by sensor %d is %.2f at %ld\n\n", sensor->room_id, sensor->sensor_id, sensor->running_avg, sensor->last_modified);
            char message[140];
            sprintf(message, "\n\t[2 - DATAMGR THREAD] Temperature increased in room %d. \n\tThe average temperature recorded by sensor %d is %.2f\n\n", sensor->room_id, sensor->sensor_id, sensor->running_avg);
            //log_event(message);
        }
        
        // pthread_cond_signal(&buffer->buffer_has_data2);   
        // ret = pthread_mutex_unlock(&buffer->sbuffer_mutex);
        // if (ret != 0) { 
        //     printf("Error locking mutex: %s\n", strerror(ret)); // Error handling
        // } else log_msg("[2 - DATAMGR THREAD] UNLOCKED HERE \n");
        
    }
    // -------------[LOG PROCESS]-------------------
    sprintf(message, "\n\t[2 - DATAMGR THREAD] Shutting down!!!\n\n");
    //log_event(message);

    pthread_exit(NULL);
    fclose(fp_data_log);
    free(sensor_list);
    free(sensor);
    
    return NULL;
}

void datamgr_free()
{
    dpl_free(&sensor_list, true);
}

void sensor_print_updated(sbuffer_t* buffer, sensor_list_t *sensor_list, double value)
{
    // pthread_mutex_t print_mutex;
    // pthread_mutex_init(&print_mutex, NULL);
    

    pthread_mutex_lock(&(buffer->sbuffer_mutex));
    if(!sensor_list){
        printf("NULL LIST \n");
        return;
    }
    int counter = 0;
    sensor_t* sensor = malloc(sizeof(sensor_t));
    dplist_node_t* tmp = sensor_list->head;
    printf ("SIZE NOW IS %d\n", dpl_size(sensor_list));
    if (!tmp) {
        printf ("Head pointing to NULL\n");
        return;
    }
    counter = 1;
    while(tmp){
        sensor = tmp->element;
        if(sensor->temp[RUN_AVG_LENGTH-1] == value){
        printf("[%d] \n",counter++);
        time_t t = sensor->last_modified;
        struct tm *tm = localtime(&t);
        if(tm){
            char* time_str = malloc(20 * sizeof(char));
            sprintf(time_str, "[%02d:%02d:%02d]: ", tm->tm_hour, tm->tm_min, tm->tm_sec);
            printf("Last modified is at : %s \n", time_str);
        }
        printf("Room ID : %d \n", sensor->room_id);
        printf("Sensor ID : %hd \n", sensor->sensor_id);
        for(int i=0;i<RUN_AVG_LENGTH;i++)
            printf("\tValue_recoded : %.2f \n", sensor->temp[i]);
        printf("Running Average : %.2f \n", sensor->running_avg);   
        tmp = tmp->next;
        }
    }
    free(sensor);
    free(tmp);
    pthread_mutex_unlock(&(buffer->sbuffer_mutex));
    // pthread_mutex_destroy(&print_mutex);
}

uint16_t datamgr_get_room_id(sensor_list_t *sensor_list, sensor_id_t sensor_id)
{
    DPLIST_ERR_HANDLER(&sensor_id == NULL, DPLIST_MEMORY_ERROR);
    sensor_t *elem = NULL;
    int booll = 0;
    for (int i = 0; i < (int)dpl_size(sensor_list); i++)
    {
        // elem = element_copy(node->element);
        // sensor_t* elem;
        elem = (sensor_t *)dpl_get_element_at_index(sensor_list, i);
        if (elem->sensor_id == sensor_id)
        {
            printf("found \n");
            booll = 1;
            // elem_room = elem->room_id;
            // return elem->room_id;
            // memcpy(elem1, elem, sizeof(sensor_t*));
            // return elem->room_id;
            break;
        }
        else
        {
            continue;
        }
        // exit(EXIT_FAILURE);
        // else node = node->next;
    }
    if (booll == 1)
        return elem->room_id;
    else
        exit(EXIT_FAILURE);
}

sensor_value_t datamgr_get_avg(sensor_list_t *sensor_list, sensor_id_t sensor_id)
{
    DPLIST_ERR_HANDLER(&sensor_id == NULL, DPLIST_MEMORY_ERROR);
    sensor_t *elem1 = NULL;
    for (int i = 0; i < (int)dpl_size(sensor_list); i++)
    {
        sensor_t *elem;
        elem = dpl_get_element_at_index(sensor_list, i);
        if (elem->sensor_id == sensor_id)
        {
            memcpy(elem1, elem, sizeof(sensor_t)); // return elem->running_avg;
        }
        else
            exit(EXIT_FAILURE);
    }
    return elem1->running_avg;
}

sensor_value_t datamgr_compute_avg(sensor_t *sensor_data, sensor_id_t sensor_id)
{
    DPLIST_ERR_HANDLER(&sensor_id == NULL, DPLIST_MEMORY_ERROR);
    double sum = 0;
    int count = 0;

    for (int i = 0; i < RUN_AVG_LENGTH; i++)
    {
        double temp = sensor_data->temp[i];
        sum += temp;
        if ((sensor_data->temp[i]) != 0)
            count++;
    }
    if (count == RUN_AVG_LENGTH)
        return sum / count;
    // else return 0;
    else
        exit(EXIT_FAILURE);
}

time_t datamgr_get_last_modified(sensor_list_t *sensor_list, sensor_id_t sensor_id)
{
    DPLIST_ERR_HANDLER(&sensor_id == NULL, DPLIST_MEMORY_ERROR);
    sensor_t *elem1 = NULL;
    for (int i = 0; i < (int)dpl_size(sensor_list) + 1; i++)
    {
        sensor_t *elem;

        elem = dpl_get_element_at_index(sensor_list, i);
        if (elem->sensor_id == sensor_id) // return elem->last_modified;
            memcpy(elem1, elem, sizeof(sensor_t));
        else
            exit(EXIT_FAILURE);
    }
    return elem1->last_modified;
    // return 0;
}

int datamgr_get_total_sensors(sensor_list_t *sensor_list)
{
    DPLIST_ERR_HANDLER(sensor_list == NULL, DPLIST_MEMORY_ERROR);
    if (!sensor_list)
    {
        printf("NULL LIST \n");
        return 0;
    }
    // int counter = 0;
    sensor_node_t *tmp = sensor_list->head;
    sensor_node_t *tmp1 = NULL;
    // int total = 0;
    int status = 0;
    int counter = 1;
    if (!tmp)
    {
        printf("Head pointing to NULL\n");
        return 0;
    }
    int total = 0;
    while (tmp)
    {
        tmp1 = sensor_list->head;
        sensor_t *elem = tmp->element;
        sensor_t *elem1 = tmp1->element;
        while (tmp1 && counter <= 0)
        {
            if (tmp1 != tmp && elem1->sensor_id == elem->sensor_id)
            {
                counter++;
            }
            tmp1 = tmp1->next;
        }
        if (counter == 1)
        {
            status = 1;
            total++;
            printf(" %u ", elem->sensor_id);
        }
        // counter = 1;
        // next node
        tmp = tmp->next;
    }
    if (status == 0)
    {
        printf(" None ");
    }
    printf("\n");
    return total;
}

void *element_copy(void *element)
{
    // check for null pointer
    if (!element)
        return NULL;
    sensor_t *sensor_element = (sensor_t *)element;
    // TODO: TO TRY memcpy
    sensor_t *element_copy;
    memcpy(&element_copy, &sensor_element, sizeof(sensor_element));
    return (void *)element_copy;
}

void element_free(void **element)
{
    // pointer check
    if (!(*element))
        return;
    sensor_t *element_to_free = *element;
    free(element_to_free);
}

int element_compare(void *x, void *y)
{
    if (!x || !y)
        return -1;
    sensor_t *sensor_y = (sensor_t *)y;
    sensor_t *sensor_x = (sensor_t *)x;
    if (sensor_x->sensor_id == sensor_y->sensor_id)
        return 0;
    else if (sensor_x->sensor_id > sensor_y->sensor_id)
        return 1;
    else
        return -1;
}

