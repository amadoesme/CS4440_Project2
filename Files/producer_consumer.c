/*
   Students: Esmeralda Amado, Cristian Hernandez Juan, Gustavo Trejo
   Class: CS 4440 - Operating Systems
   Description: producer/consumer problem
*/

// import libraries
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define BUFFER_SIZE 10
#define TOTAL_ITEMS 50

// shared buffer
char buffer[BUFFER_SIZE];

// indexes for circular buffer
int in = 0;
int out = 0;

// current number of items
int bufferCount = 0;

// synchronization objects
pthread_mutex_t mutex;
sem_t emptySlots;
sem_t fullSlots;

void *producer(void *arg)
{
    for (int i = 0; i < TOTAL_ITEMS; i++)
    {
        // create a printable character
        char item = 'A' + (i % 26);

        // check if buffer is full
        int value;
        if (sem_getvalue(&emptySlots, &value) == 0 && value == 0)
            printf("Producer waiting (buffer full)...\n");

        // wait for an empty slot
        sem_wait(&emptySlots);

        // enter critical section
        pthread_mutex_lock(&mutex);

        // add item to buffer
        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        bufferCount++;

        // print produced item
        printf("Produced: %c | Buffer items: %d\n", item, bufferCount);

        // leave critical section
        pthread_mutex_unlock(&mutex);

        // signal there is a new item
        sem_post(&fullSlots);

        // producer is faster
        usleep(100000);
    }

    return NULL;
}

void *consumer(void *arg)
{
    for (int i = 0; i < TOTAL_ITEMS; i++)
    {
        // check if buffer is empty
        int value;
        if (sem_getvalue(&fullSlots, &value) == 0 && value == 0)
            printf("Consumer waiting (buffer empty)...\n");

        // wait for an available item
        sem_wait(&fullSlots);

        // enter critical section
        pthread_mutex_lock(&mutex);

        // remove item from buffer
        char item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        bufferCount--;

        // print consumed item
        printf("Consumed: %c | Buffer items: %d\n", item, bufferCount);

        // leave critical section
        pthread_mutex_unlock(&mutex);

        // signal there is an empty slot
        sem_post(&emptySlots);

        // consumer is slower
        usleep(500000);
    }

    return NULL;
}

int main()
{
    pthread_t producerThread;
    pthread_t consumerThread;

    // initialize mutex
    pthread_mutex_init(&mutex, NULL);

    // buffer starts empty
    sem_init(&emptySlots, 0, BUFFER_SIZE);
    sem_init(&fullSlots, 0, 0);

    // start consumer first
    pthread_create(&consumerThread, NULL, consumer, NULL);

    // let consumer wait on empty buffer
    sleep(1);

    // start producer
    pthread_create(&producerThread, NULL, producer, NULL);

    // wait for both threads
    pthread_join(producerThread, NULL);
    pthread_join(consumerThread, NULL);

    // clean up
    pthread_mutex_destroy(&mutex);
    sem_destroy(&emptySlots);
    sem_destroy(&fullSlots);

    printf("All items produced and consumed.\n");

    return 0;
}