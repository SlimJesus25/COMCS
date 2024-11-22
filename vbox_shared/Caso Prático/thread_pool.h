#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define MAX_THREADS 4

pthread_mutex_t mutex;
pthread_cond_t condVar;

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