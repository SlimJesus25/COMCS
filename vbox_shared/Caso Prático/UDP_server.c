#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <regex.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <cjson/cJSON.h> // sudo apt install libcjson1 libcjson-dev | use compilation flag ""
#include "MQTTClient.h"  // sudo apt-get install libpaho-mqtt-dev | use compilation flag "lpaho-mqtt3c"
#include "avl.h"
#include "queue.h"
#include "utils.h"
#include "smart_data.h"

/*
 To add compilation flags to a VS Code environment, hit CTRL+SHIFT+P and type tasks, select your compiler (gcc) and insert into the "args".
*/

#define BUF_SIZE 999
#define SERVER_PORT "9999"
#define MAX_THREADS 4
#define MAX_TASKS 10
#define MQTT_BROKER_ADDRESS "192.168.1.154:1883"
#define ID "alert_server"
#define MQTT_PAYLOAD 100
#define MQTT_TOPIC "server/alert"
#define MQTT_TIMEOUT 10000L
#define JSON_STRING 0
#define JSON_DOUBLE 1
#define SMART_DATA_FIELDS 6
#define MAX_BOUND_TEMPERATURE 50
#define MIN_BOUND_TEMPERATURE 0
#define MIN_BOUND_HUMIDITY 20
#define MAX_BOUND_HUMIDITY 80
#define MARGIN_ERROR_MS 50
#define QOS_AT_LEAST_ONCE 1
#define QOS_EXACTLY_ONCE 2

/*
TODO:
1. Create a configuration file to define buffer size, server port, max threads and tasks (for thread pool).
2. Create a header file for the thread pool code.
*/

/*
The connection structure, defines the user and the QoS that this user wants.
The range of available QoS goes from 0 to 2, where:
 - 0 - Best Effort;
 - 1 - At Least Once;
 - 2 - Guaranteed.
*/
typedef struct{
    char *user;
    int qos;
} connection_t;

// Task structure.
typedef struct{
    void (*function)(void *arg);
    void *arg;
} task_t;

// Thread pool structure.
typedef struct{
    pthread_t *threads;
    task_t *taskQueue;
    int taskQueueSize;
    int taskQueueFront;
    int taskQueueRear;
    int taskCount;
    pthread_mutex_t mutex;
    pthread_cond_t condVar;
    int stop;
} ThreadPool;

struct BinaryTreeNode *database;
struct BinaryTreeNode *packet_database;
pthread_mutex_t stdout_mutex;
pthread_mutex_t tree_size_mutex;
pthread_cond_t signal;
int received_values;
Queue queue;
MQTTClient mqtt_client;
int qos;

char fields[SMART_DATA_FIELDS][20] = {{"type\0"}, {"id\0"}, {"dateObserved\0"}, {"location\0"}, {"temperature\0"}, {"relativeHumidity\0"}};
int types[SMART_DATA_FIELDS] = {JSON_STRING, JSON_STRING, JSON_STRING, JSON_STRING, JSON_DOUBLE, JSON_DOUBLE};

void retrieve_data_from_json(cJSON* json, int type, char* field, smart_data_t* conv_arg){

    cJSON *attribute = cJSON_GetObjectItemCaseSensitive(json, field);
    switch(type){
        case JSON_STRING:
            if (cJSON_IsString(attribute) && (attribute->valuestring != NULL)){
                if(strcmp(field, "dateObserved") == 0){
                    conv_arg->dateObserved = (char *)calloc(strlen((char*)attribute->valuestring), sizeof(char));
                    strcpy(conv_arg->dateObserved, attribute->valuestring);
                }else if(strcmp(field, "id") == 0){
                    conv_arg->id = (char *)calloc(strlen((char*)attribute->valuestring), sizeof(char));
                    strcpy(conv_arg->id, attribute->valuestring);
                }else if(strcmp(field, "location") == 0){
                    conv_arg->location = (char *)calloc(strlen((char*)attribute->valuestring), sizeof(char));
                    strcpy(conv_arg->location, attribute->valuestring);
                }else{
                    conv_arg->type = (char *)calloc(strlen((char*)attribute->valuestring), sizeof(char));
                    strcpy(conv_arg->type, attribute->valuestring);
                }
            }
            break;

        case JSON_DOUBLE:
            if (cJSON_IsNumber(attribute)){
                if(strcmp(field, "temperature") == 0)
                    conv_arg->temperature = attribute->valuedouble;
                else conv_arg->humidity = attribute->valuedouble;
            }
            break;
    }
}

void publish_mqtt(char* alert){
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    pubmsg.payload = alert;
    pubmsg.payloadlen = strlen(alert);
    pubmsg.qos = 0;
    pubmsg.retained = 0;

    int rv;
    if((rv = MQTTClient_publishMessage(mqtt_client, MQTT_TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS){
        char msg[strlen("Message was not published to the command center. Error Code: ") + 2];
        sprintf(msg, "Message was not published to the command center. Error Code: %d", rv);
        log_error(msg);
    }
    // rc = MQTTClient_waitForCompletion(mqtt_client, token, MQTT_TIMEOUT);
}

void alert_anomaly(double value, int bound, char* verb){
    char* alert = (char*)calloc(100, sizeof(char));
    sprintf(alert, "Temperature %s the usual rate of %d with %.2f", verb, bound, value);
    log_warning(alert);
    publish_mqtt(alert);
}

void alert_anomaly_generic(char* alert){
    log_warning(alert);
    publish_mqtt(alert);
}

void calculate_difference(double* temperature, double* humidity, int size){

    if(size == 1){
        if(*(temperature) > MAX_BOUND_TEMPERATURE)
            alert_anomaly(*(temperature), MAX_BOUND_TEMPERATURE, "is above");
        else if(*(temperature) < MIN_BOUND_TEMPERATURE)
            alert_anomaly(*(temperature), MIN_BOUND_TEMPERATURE, "is below");
        
        if(*(humidity) > MAX_BOUND_HUMIDITY)
            alert_anomaly(*(humidity), MAX_BOUND_HUMIDITY, "is above");
        else if(*(humidity) < MIN_BOUND_HUMIDITY)
            alert_anomaly(*(humidity), MIN_BOUND_HUMIDITY, "is below");
        return;
    }

    double tempDiff, humiDiff;
    for(int i=0;i<=size/2;i++){
        for(int j=i+1;j<size;j++){
            tempDiff = abs(*(temperature+i) - *(temperature+j));
            humiDiff = abs(*(humidity+i) - *(humidity+j));

            if(tempDiff > MAX_BOUND_TEMPERATURE)
                alert_anomaly(tempDiff, MAX_BOUND_TEMPERATURE, "is above");
            else if(tempDiff < MIN_BOUND_TEMPERATURE)
                alert_anomaly(tempDiff, MIN_BOUND_TEMPERATURE, "is below");
            
            if(humiDiff > MAX_BOUND_HUMIDITY)
                alert_anomaly(humiDiff, MAX_BOUND_HUMIDITY, "is above");
            else if(humiDiff < MIN_BOUND_HUMIDITY)
                alert_anomaly(humiDiff, MIN_BOUND_HUMIDITY, "is below");
        }
    }
}

/**
 * This function is responsible for retrieve data from the sensors, 
 * calculate the difference between temperature and humidity values
 * and alert the central command if something is out of bounds.
 */
void* handle_client_request_master(void *arg){
    while(1){

        while(tree_size() == 0)
            sleep(1);

        // This barrier algorithm, ensures that this (master) thread
        // is going to read, only if the values are available.
        pthread_mutex_lock(&tree_size_mutex);
        while(queue_size(&queue) < tree_size())
            pthread_cond_wait(&signal, &tree_size_mutex);
                
        
        int size = tree_size();
        smart_data_t* items = (smart_data_t*)calloc(size, sizeof(smart_data_t));
        for(int i=0;i<size;i++){
            *(items+i) = *(peek(&queue));
            dequeue(&queue);
        }
        pthread_mutex_unlock(&tree_size_mutex);

        // If the queue has as much items as the database size, but 
        // one of the sensors is broken, it means that an alert must
        // be sent. We check if the sensors are transmitting if the 
        // first "size" items in the queue have a max margin error. 

        double temperatures[size], humidities[size];

        int sensor_degraded = 0;
        for(int i=0;i<=size/2;i++){
            temperatures[i] = (items+i)->temperature;
            humidities[i] = (items+i)->humidity;
            for(int j=i+1;j<size;j++)
                if(abs((items+i)->timestamp - (items+j)->timestamp) > MARGIN_ERROR_MS){
                    alert_anomaly_generic("A sensor is degraded");
                    sensor_degraded = 1;
                    break;
                }
            if(sensor_degraded)
                break;
        }
        if(sensor_degraded)
            continue;    
        
        calculate_difference(temperatures, humidities, size);
        

        // pthread_mutex_unlock(&tree_size_mutex);
        pthread_mutex_lock(&stdout_mutex);
        
        pthread_mutex_unlock(&stdout_mutex);
    }
}

void handle_client_request_slave(void *arg){

    // 1. Casts the void (bytes) to char*.
    char *line1 = (char *)arg;

    // 1. Parses the received data into a JSON object.
    cJSON *json = cJSON_Parse(line1);
    if (json == NULL){
        const char *error_ptr = cJSON_GetErrorPtr();
        if(error_ptr != NULL)
            log_error("Invalid JSON structure");
        else log_error("Unknown JSON error");
        cJSON_Delete(json);
        return;
    }
    
    // 2. Allocates memory to the argument (that fits into smart data standards).
    smart_data_t *conv_arg;
    conv_arg = calloc(1, sizeof(smart_data_t));

    for(int i=0;i<SMART_DATA_FIELDS;i++)
        retrieve_data_from_json(json, types[i], fields[i], conv_arg);

    enqueue(&queue, *(conv_arg));

    pthread_cond_signal(&signal);

    // 4. Deletes and frees unused variables.
    cJSON_Delete(json);
}

// Worker thread function
void *workerThread(void *arg){
    ThreadPool *pool = (ThreadPool *)arg;
    while (1){
        pthread_mutex_lock(&pool->mutex);

        // Wait for a task to become available or stop signal
        while (pool->taskCount == 0 && !pool->stop){
            pthread_cond_wait(&pool->condVar, &pool->mutex);
        }

        if (pool->stop){
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        // Get the task from the queue
        task_t task = pool->taskQueue[pool->taskQueueFront];
        pool->taskQueueFront = (pool->taskQueueFront + 1) % pool->taskQueueSize;
        pool->taskCount--;

        pthread_mutex_unlock(&pool->mutex);

        // Execute the task
        task.function(task.arg);
    }

    return NULL;
}

// Function to initialize the thread pool
void threadPoolInit(ThreadPool *pool, int numThreads, int maxTasks){
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * numThreads);
    pool->taskQueue = (task_t *)malloc(sizeof(task_t) * maxTasks);
    pool->taskQueueSize = maxTasks;
    pool->taskQueueFront = 0;
    pool->taskQueueRear = 0;
    pool->taskCount = 0;
    pool->stop = 0;
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->condVar, NULL);

    // Create the threads
    for (int i = 0; i < numThreads; i++){
        pthread_create(&pool->threads[i], NULL, (void *(*)(void *))workerThread, pool);
    }
}

// Function that allows the caller to submit a task to the thread pool.
void threadPoolSubmit(ThreadPool *pool, void (*function)(void *), char *arg){

    // 1. Locks the mutual exclusion area.
    pthread_mutex_lock(&pool->mutex);

    // 2. While the pool is full, the caller thread waits on the condition variable. Whenever the condition variable receives a signal
    // the thread wakes up, and verifies if there is "space in the pool" to allocate a new thread.
    // It's noteworthy to say that, whenever the thread that executes the condition variable, releases the mutex.
    // And when this thread wakes up, it will try to adquire the lock.
    while (pool->taskCount == pool->taskQueueSize)
        pthread_cond_wait(&pool->condVar, &pool->mutex);

    // 3. Add the task to the queue
    pool->taskQueue[pool->taskQueueRear].function = function;
    pool->taskQueue[pool->taskQueueRear].arg = arg;
    pool->taskQueueRear = (pool->taskQueueRear + 1) % pool->taskQueueSize;
    pool->taskCount++;

    // 4. Signal that a task is available
    pthread_cond_signal(&pool->condVar);

    // 5. Unlocks the mutual exclusion area.
    pthread_mutex_unlock(&pool->mutex);
}

// Stop, destroy and clean up the thread pool.
void threadPoolDestroy(ThreadPool *pool){
    pthread_mutex_lock(&pool->mutex);
    pool->stop = 1;
    pthread_cond_broadcast(&pool->condVar); // Wake all threads to terminate
    pthread_mutex_unlock(&pool->mutex);

    // Join all threads
    for (int i = 0; i < MAX_THREADS; i++){
        pthread_join(pool->threads[i], NULL);
    }

    // Free resources
    free(pool->threads);
    free(pool->taskQueue);
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->condVar);
}

int main(void){
    struct sockaddr_storage client;
    int err, sock, res, i;
    unsigned int adl;
    char line[BUF_SIZE], line1[BUF_SIZE];
    char cliIPtext[BUF_SIZE], cliPortText[BUF_SIZE];
    struct addrinfo req, *list;
    bzero((char *)&req, sizeof(req));

    if(pthread_mutex_init(&tree_size_mutex, NULL) < 0){
        log_error("Mutex was not created");
        exit(EXIT_FAILURE);
    }
    
    if(pthread_cond_init(&signal, NULL) < 0){
        log_error("Condition variable was not created");
        exit(EXIT_FAILURE);
    }

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    MQTTClient_create(&mqtt_client, MQTT_BROKER_ADDRESS, ID,
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);
    
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(mqtt_client, &conn_opts)) != MQTTCLIENT_SUCCESS){
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }

    initializeQueue(&queue);

    pthread_t master;
    pthread_create(&master, NULL, handle_client_request_master, NULL);

    // request a IPv6 local address will allow both IPv4 and IPv6 clients to use it
    req.ai_family = AF_INET6;
    req.ai_socktype = SOCK_DGRAM;
    req.ai_flags = AI_PASSIVE; // local address
    err = getaddrinfo(NULL, SERVER_PORT, &req, &list);
    srand(time(NULL));

    

    // Initialize thread pool.
    ThreadPool pool;
    threadPoolInit(&pool, MAX_THREADS, MAX_TASKS);

    if (err){
        printf("Failed to get local address, error: %s\n", gai_strerror(err));
        exit(1);
    }

    sock = socket(list->ai_family, list->ai_socktype, list->ai_protocol);

    if (sock == -1){
        perror("Failed to open socket");
        freeaddrinfo(list);
        exit(1);
    }

    if (bind(sock, (struct sockaddr *)list->ai_addr, list->ai_addrlen) == -1){
        perror("Bind failed");
        close(sock);
        freeaddrinfo(list);
        exit(1);
    }

    freeaddrinfo(list);
    log_info("Alert Server is now running and listening for UDP requests (IPv6/IPv4)");
    adl = sizeof(client);

    while (1){

        // 1. Receives data from client(s).
        res = recvfrom(sock, line, BUF_SIZE, 0, (struct sockaddr *)&client, &adl);

        if (getnameinfo((struct sockaddr *)&client, adl, cliIPtext, BUF_SIZE, cliPortText, BUF_SIZE, NI_NUMERICHOST | NI_NUMERICSERV)){
            puts("Got request, but failed to get client address");
            continue;
        }

        kvalue_t entry;
        entry.key = (char *)calloc(strlen(cliIPtext), sizeof(char));
        strcpy(entry.key, cliIPtext);

        // QoS is defined as 'Best Effort' by default.
        struct BinaryTreeNode *node;

        // 2. Creates a mirror of the text line.
        for (i = 0; i < res; i++)
            line1[i] = line[i];

        // printf("\nLine1: %s\n", line1);

        // If it's the first value, we initialize the database. The "entry.value = atoi(line1);" is present in the first two conditions,
        // because it means that is going to be the first request from a new client. The first request always have the QoS that the client
        // wants to establish.
        if (database == NULL || (node = searchNode(database, cliIPtext)) == NULL){ // If the node is not found, we insert it.
            entry.value = atoi(line1);
            database = insertNode(database, entry);
            char msg[40];
            sprintf(msg, "New client %s with QoS %d", entry.key, entry.value);
            log_info(msg);
            continue;
        }else // If the node was found, we set the QoS to the specified.
            qos = node->key.value;

        if(qos == QOS_AT_LEAST_ONCE){
            sendto(sock, "ACK\n", 4, 0, (struct sockaddr *)&client, adl); // Flags (like MSG_CONFIRM) does not work properly with UDP.
        }else if(qos == QOS_EXACTLY_ONCE){
            if(searchNode(packet_database, cliPortText) != NULL){
                sendto(sock, "APP\n", 4, 0, (struct sockaddr *)&client, adl); // APP -> Already processed.
                continue;
            }
            kvalue_t pack;
            pack.key = (char*)calloc(strlen(cliPortText), sizeof(char));
            strcpy(pack.key, cliPortText);
            pack.value = atoi(cliPortText);
            packet_database = insertNode(packet_database, pack);
            sendto(sock, "ACK\n", 4, 0, (struct sockaddr *)&client, adl);

            /* Até que ponto é necessário fazer exatamente igual ao MQTT? Ao colocar um recvfrom, este pode ser interceptado por
            outro sensor estragando o esquema. Ter uma "base de dados" com os identificadores dos pacotes, já faz bem o trabalho.

            char* pubrel = (char*)calloc(7, sizeof(char));
            recvfrom(sock, pubrel, 7, 0, (struct sockaddr *)&client, &adl);
            
            sendto(sock, "PUBCOMP\n", 8, 0, (struct sockaddr *)&client, adl);
            */
        }

        // 3. Submits the request to a thread from the previously initializated thread pool.
        threadPoolSubmit(&pool, handle_client_request_slave, line1);
    }

    // unreachable, but a good practice
    close(sock);
    exit(0);
}
