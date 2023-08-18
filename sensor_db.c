#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sqlite3.h>

#include "config.h"
#include "sensor_db.h"

#ifndef BUFFSIZE
#define BUFFSIZE 50
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 264
#endif

#define LOOPS 5
#define CHILD_POS "\t\t\t"


int f(void *unused, int count, char **data, char **columns)
{
    int idx;
    printf("There are %d column(s)\n", count);

    for (idx = 0; idx < count; idx++) {
        printf("The data in column \"%s\" is: %s\n", columns[idx], data[idx]);      
    }
    printf("\n");
    return 0;
}

void run_child(sbuffer_t* buffer, DBCONN *conn, int pipefd[2], int exit_code){

        // Connected to db - Not connected to db
        if(conn) {
        // -------------[LOG PROCESS]-------------------
            char message[100];
            sprintf(message, "[3 - StormgrID]: Connected to SQL database\n");
            log_event(message); 
        } else {
            char message[100];
            sprintf(message, "[3 - StormgrID]: Unable to connect to SQL database\n");
            log_event(message);
        }
        sleep(1);

        //printf(CHILD_POS "Child process (pid = %d) of parent (pid = %d) is terminating ... \n", my_pid, parent_pid);

        printf("[3 - StoremgrID] Child run\n\n");
            while(!buffer->exit_flag){
    }
    fprintf(stderr, "[3 - StoremgrID] Closed connection SQL database");
    
    sqlite3_close(conn);
    pthread_exit(NULL);
    //return NULL;
}

DBCONN *init_connection(char clear_up_flag){
    char* err;
    sqlite3* db;
    
    if(clear_up_flag==1){
        char query[264];
        sqlite3_open(TO_STRING(DB_NAME), &db);
        //sqlite3_free(db);
        sprintf(query, "DROP TABLE IF EXISTS %s AND CREATE TABLE IF NOT EXISTS %s(id INTEGER PRIMARY KEY AUTOINCREMENT, sensor_id INT, sensor_value DECIMAL(4,2), timestamp VARCHAR(60));", TO_STRING(TABLE_NAME), TO_STRING(TABLE_NAME));        
        //int ret = sqlite3_exec(db, query, log_callback, NULL, &err);
        int ret = 0;
        if(ret != SQLITE_OK){
            printf("[ERR]: mché inayek lenna \n");
            return NULL;
        }       
    }
    else {
        sqlite3_open(TO_STRING(DB_NAME), &db);
        int ret = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS SensorData(id INTEGER PRIMARY KEY AUTOINCREMENT, sensor_id INT, sensor_value DECIMAL(4,2), timestamp VARCHAR(60));", f, NULL, &err);
        if(ret != SQLITE_OK){
            fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
            //sqlite3_free(db);
            return NULL;
        }
    }
    printf("[SUCCESS] \n");
    return db;
}

void disconnect(DBCONN *conn){
    sqlite3_close(conn);
    if(!conn) fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(conn)); 
    else printf("Disconnected\n");
}

int insert_sensor(DBCONN *conn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts){
    char* err;
    char query[140];

    sprintf(query, "INSERT INTO %s (sensor_id, sensor_value, timestamp) VALUES(%u, %f, %ld)", TO_STRING(TABLE_NAME), id, value, ts);
    int ret = sqlite3_exec(conn, query, NULL, NULL, &err);
    
    if(ret != SQLITE_OK){
        fprintf(stderr, "Cannot %s\n", err);
        return 1;
    }
    return 0;
}

int insert_sensor_from_file(DBCONN *conn, FILE *sensor_data){
    uint16_t buffer_sensor_id;
    double buffer_temp_value;
    time_t buffer_ts;
    int idx = 0;
    while(fread(&buffer_sensor_id, sizeof(buffer_sensor_id), 1, sensor_data) > 0){
        fread(&buffer_temp_value, sizeof(buffer_temp_value), 1, sensor_data);
        fread(&buffer_ts, sizeof(buffer_ts), 1, sensor_data);
        idx++;
        int ret= insert_sensor(conn, buffer_sensor_id, buffer_temp_value, buffer_ts);
        if(ret != 0) {return -1;}
        }
        printf("row %d\n", idx);
        printf("SUCCESS! ALL DATA HAS BEEN STORED INTO THE DATABASE. \n");
        return 0;
}

int find_sensor_all(DBCONN *conn, callback_t log_callback){
    char* err; 
    char query[60];
    sprintf(query, "SELECT sensor_value FROM %s;", TO_STRING(TABLE_NAME));
    int ret = sqlite3_exec(conn, query, log_callback, 0, &err);
    
    if(ret != SQLITE_OK){
        fprintf(stderr, "Error: Cannot select find data! %s\n", err);
        return 1;
    }
    printf("DATA SELECTED OK FOR ALL \n");
    return 0;    
}

int find_sensor_by_value(DBCONN *conn, sensor_value_t value, callback_t log_callback){
    char* err; 
    char query[60];
    sprintf(query, "SELECT * FROM %s WHERE sensor_value = '%f';", TO_STRING(TABLE_NAME), value);
    int ret = sqlite3_exec(conn, query, f, 0, &err);
    
    if(ret != SQLITE_OK){
        fprintf(stderr, "Error: Could not retrieve value from database: %s\n", err);
        return 1;
    }
    printf("DATA SELECTED OK FOR VALUE \n");
    return 0;    
}

int find_sensor_exceed_value(DBCONN *conn, sensor_value_t value, callback_t log_callback){
    char* err; 
    char query[60];
    sprintf(query, "SELECT * FROM %s WHERE sensor_value > '%f';", TO_STRING(TABLE_NAME), value);
    int ret = sqlite3_exec(conn, query, f, 0, &err);
    
    if(ret != SQLITE_OK){
        fprintf(stderr, "Error: Could not retrieve any values: %s\n", err);
        return 1;
    }
    printf("DATA SELECTED OK FOR VALUE BIG THAN MAX \n");
    return 0;    
}

int find_sensor_by_timestamp(DBCONN *conn, sensor_ts_t ts, callback_t log_callback){
    char* err; 
    char query[60];
    sprintf(query, "SELECT * FROM %s WHERE timestamp = '%ld';", TO_STRING(TABLE_NAME), ts);
    int ret = sqlite3_exec(conn, query, f, 0, &err);
    
    if(ret != SQLITE_OK){
        fprintf(stderr, "Error: Could not retrieve any values: %s\n", err);
        return 1;
    }
    printf("DATA SELECTED OK FOR ts \n");
    return 0;    
}

int find_sensor_after_timestamp(DBCONN *conn, sensor_ts_t ts, callback_t log_callback){
    char* err; 
    char query[60];
    sprintf(query, "SELECT * FROM %s WHERE timestamp > '%ld';", TO_STRING(TABLE_NAME), ts);
    int ret = sqlite3_exec(conn, query, f, 0, &err);
    
    if(ret != SQLITE_OK){
        fprintf(stderr, "Error: Could not retrieve any values: %s\n", err);
        return 1;
    }
    printf("DATA SELECTED OK FOR BIGGER THAN ts \n");
    return 0;    
}


