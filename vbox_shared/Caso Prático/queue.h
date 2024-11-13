#include <stdio.h>
#include <stdbool.h>
#include "smart_data.h"

#define MAX_SIZE 100

typedef struct {
     smart_data_t items[MAX_SIZE];
     int rear;
     int front;
     int size;
} Queue;

// Function to initialize the queue
void initializeQueue(Queue* q){
    q->front = -1;
    q->rear = 0;
    q->size = 0;
}

// Function to check if the queue is empty
bool isEmpty(Queue* q) { return (q->front == q->rear - 1); }

// Function to check if the queue is full
bool isFull(Queue* q) { return (q->rear == MAX_SIZE); }

// Function to add an element to the queue (Enqueue
// operation)
void enqueue(Queue* q, smart_data_t value)
{
    if (isFull(q)) {
        printf("Queue is full\n");
        return;
    }
    q->items[q->rear] = value;
    q->rear++;
    q->size++;
}

// Function to remove an element from the queue (Dequeue
// operation)
void dequeue(Queue* q)
{
    if (isEmpty(q)) 
        return;
    
    q->front++;
    q->size--;
}

// Function to get the element at the front of the queue
// (Peek operation)
smart_data_t* peek(Queue* q){
    if (isEmpty(q))
        return NULL; 
    return &(q->items[q->front + 1]);
}

int queue_size(Queue* q){
    return q->size;
}