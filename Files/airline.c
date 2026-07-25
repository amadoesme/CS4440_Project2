/*
   Students: Esmeralda Amado, Cristian Hernandez Juan, Gustavo Trejo
   Class: CS 4440 - Operating Systems
   Description: Simulates airline passengers going through baggage,
                security, and boarding using threads and synchronization.
*/

#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// passenger information
typedef struct Passenger
{
    int id;

    // signal when each stage is complete
    sem_t baggage_done;
    sem_t security_done;
    sem_t boarding_done;
} Passenger;

// queue for each processing stage
typedef struct Stage
{
    // passenger queue
    Passenger **buf;
    int capacity;
    int head;
    int tail;
    int count;

    // protect the queue
    pthread_mutex_t mtx;
    pthread_cond_t not_empty;

    // track completed passengers
    int processed;
    int total;
    bool done;
} Stage;

// number of passengers and workers
static int P = 0;
static int B = 0;
static int S = 0;
static int F = 0;

// passenger list
static Passenger *passengers = NULL;

// processing stages
static Stage baggage_stage;
static Stage security_stage;
static Stage boarding_stage;

// prevent mixed console output
static pthread_mutex_t print_mtx = PTHREAD_MUTEX_INITIALIZER;

// print one message at a time
static void say(const char *fmt, ...)
{
    va_list ap;

    pthread_mutex_lock(&print_mtx);

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    fflush(stdout);

    pthread_mutex_unlock(&print_mtx);
}

// initialize a processing stage
static void stage_init(Stage *s, int total)
{
    // allocate the passenger queue
    s->capacity = total;
    s->buf = calloc((size_t)total, sizeof(Passenger *));

    // initialize queue values
    s->head = 0;
    s->tail = 0;
    s->count = 0;

    // initialize synchronization
    pthread_mutex_init(&s->mtx, NULL);
    pthread_cond_init(&s->not_empty, NULL);

    // initialize progress values
    s->processed = 0;
    s->total = total;
    s->done = false;
}

// free stage resources
static void stage_destroy(Stage *s)
{
    free(s->buf);
    pthread_mutex_destroy(&s->mtx);
    pthread_cond_destroy(&s->not_empty);
}

// add a passenger to a queue
static void stage_enqueue(Stage *s, Passenger *p)
{
    // lock the queue
    pthread_mutex_lock(&s->mtx);

    // add the passenger
    s->buf[s->tail] = p;
    s->tail = (s->tail + 1) % s->capacity;
    s->count++;

    // wake up one worker
    pthread_cond_signal(&s->not_empty);

    // unlock the queue
    pthread_mutex_unlock(&s->mtx);
}

// get the next passenger
static Passenger *stage_dequeue(Stage *s)
{
    pthread_mutex_lock(&s->mtx);

    // wait until a passenger arrives
    while (s->count == 0 && !s->done)
    {
        pthread_cond_wait(&s->not_empty, &s->mtx);
    }

    // stop when the stage is finished
    if (s->count == 0 && s->done)
    {
        pthread_mutex_unlock(&s->mtx);
        return NULL;
    }

    // remove the next passenger
    Passenger *p = s->buf[s->head];
    s->head = (s->head + 1) % s->capacity;
    s->count--;

    pthread_mutex_unlock(&s->mtx);

    return p;
}

// update completed passengers
static void stage_mark_processed(Stage *s)
{
    pthread_mutex_lock(&s->mtx);

    s->processed++;

    // check if everyone finished this stage
    if (s->processed == s->total)
    {
        s->done = true;

        // wake up remaining workers
        pthread_cond_broadcast(&s->not_empty);
    }

    pthread_mutex_unlock(&s->mtx);
}

// baggage handler thread
static void *baggage_handler(void *arg)
{
    (void)arg;

    while (1)
    {
        // wait for a passenger
        Passenger *p = stage_dequeue(&baggage_stage);

        // stop when the stage is finished
        if (p == NULL)
        {
            break;
        }

        // simulate baggage processing
        usleep(100);

        // signal the passenger
        sem_post(&p->baggage_done);

        // update stage progress
        stage_mark_processed(&baggage_stage);
    }

    return NULL;
}

// security screener thread
static void *security_screener(void *arg)
{
    (void)arg;

    while (1)
    {
        // wait for a passenger
        Passenger *p = stage_dequeue(&security_stage);

        // stop when the stage is finished
        if (p == NULL)
        {
            break;
        }

        // simulate security screening
        usleep(100);

        // signal the passenger
        sem_post(&p->security_done);

        // update stage progress
        stage_mark_processed(&security_stage);
    }

    return NULL;
}

// flight attendant thread
static void *flight_attendant(void *arg)
{
    (void)arg;

    while (1)
    {
        // wait for a passenger
        Passenger *p = stage_dequeue(&boarding_stage);

        // stop when the stage is finished
        if (p == NULL)
        {
            break;
        }

        // simulate boarding
        usleep(100);

        // signal the passenger
        sem_post(&p->boarding_done);

        // update stage progress
        stage_mark_processed(&boarding_stage);
    }

    return NULL;
}

// passenger thread
static void *passenger_thread(void *arg)
{
    Passenger *me = (Passenger *)arg;

    // passenger arrives
    say("Passenger #%d arrived at the terminal.\n", me->id);

    // baggage stage
    say("Passenger #%d is waiting at baggage processing for a handler.\n",
        me->id);

    stage_enqueue(&baggage_stage, me);

    // wait for baggage processing
    sem_wait(&me->baggage_done);

    // security stage
    say("Passenger #%d is waiting to be screened by a screener.\n",
        me->id);

    stage_enqueue(&security_stage, me);

    // wait for security screening
    sem_wait(&me->security_done);

    // boarding stage
    say("Passenger #%d is waiting to board the plane by an attendant.\n",
        me->id);

    stage_enqueue(&boarding_stage, me);

    // wait to be seated
    sem_wait(&me->boarding_done);

    // passenger is seated
    say("Passenger #%d has been seated and relaxes.\n", me->id);

    return NULL;
}

// print usage information
static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s P B S F\n", prog);
    fprintf(stderr, "Example: %s 100 3 5 2\n", prog);
}

int main(int argc, char **argv)
{
    // check command line arguments
    if (argc != 5)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // read actor counts
    P = atoi(argv[1]);
    B = atoi(argv[2]);
    S = atoi(argv[3]);
    F = atoi(argv[4]);

    // make sure all values are positive
    if (P <= 0 || B <= 0 || S <= 0 || F <= 0)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // initialize processing stages
    stage_init(&baggage_stage, P);
    stage_init(&security_stage, P);
    stage_init(&boarding_stage, P);

    // check stage memory
    if (baggage_stage.buf == NULL ||
        security_stage.buf == NULL ||
        boarding_stage.buf == NULL)
    {
        fprintf(stderr, "Error: unable to allocate stage queues.\n");
        return EXIT_FAILURE;
    }

    // allocate worker thread arrays
    pthread_t *b_threads =
        calloc((size_t)B, sizeof(pthread_t));

    pthread_t *s_threads =
        calloc((size_t)S, sizeof(pthread_t));

    pthread_t *f_threads =
        calloc((size_t)F, sizeof(pthread_t));

    if (b_threads == NULL ||
        s_threads == NULL ||
        f_threads == NULL)
    {
        fprintf(stderr, "Error: unable to allocate worker threads.\n");
        return EXIT_FAILURE;
    }

    // create baggage handlers
    for (int i = 0; i < B; i++)
    {
        pthread_create(&b_threads[i],
                       NULL,
                       baggage_handler,
                       NULL);
    }

    // create security screeners
    for (int i = 0; i < S; i++)
    {
        pthread_create(&s_threads[i],
                       NULL,
                       security_screener,
                       NULL);
    }

    // create flight attendants
    for (int i = 0; i < F; i++)
    {
        pthread_create(&f_threads[i],
                       NULL,
                       flight_attendant,
                       NULL);
    }

    // allocate passengers
    passengers =
        calloc((size_t)P, sizeof(Passenger));

    pthread_t *p_threads =
        calloc((size_t)P, sizeof(pthread_t));

    if (passengers == NULL || p_threads == NULL)
    {
        fprintf(stderr, "Error: unable to allocate passengers.\n");
        return EXIT_FAILURE;
    }

    // initialize passenger information
    for (int i = 0; i < P; i++)
    {
        passengers[i].id = i + 1;

        sem_init(&passengers[i].baggage_done, 0, 0);
        sem_init(&passengers[i].security_done, 0, 0);
        sem_init(&passengers[i].boarding_done, 0, 0);
    }

    // create passenger threads last
    for (int i = 0; i < P; i++)
    {
        pthread_create(&p_threads[i],
                       NULL,
                       passenger_thread,
                       &passengers[i]);
    }

    // wait for all passengers
    for (int i = 0; i < P; i++)
    {
        pthread_join(p_threads[i], NULL);
    }

    // wait for baggage handlers
    for (int i = 0; i < B; i++)
    {
        pthread_join(b_threads[i], NULL);
    }

    // wait for security screeners
    for (int i = 0; i < S; i++)
    {
        pthread_join(s_threads[i], NULL);
    }

    // wait for flight attendants
    for (int i = 0; i < F; i++)
    {
        pthread_join(f_threads[i], NULL);
    }

    // plane leaves after everyone is seated
    say("All %d passengers are seated. The plane takes off!\n", P);

    // destroy passenger semaphores
    for (int i = 0; i < P; i++)
    {
        sem_destroy(&passengers[i].baggage_done);
        sem_destroy(&passengers[i].security_done);
        sem_destroy(&passengers[i].boarding_done);
    }

    // free passenger memory
    free(passengers);
    free(p_threads);

    // free worker memory
    free(b_threads);
    free(s_threads);
    free(f_threads);

    // destroy processing stages
    stage_destroy(&baggage_stage);
    stage_destroy(&security_stage);
    stage_destroy(&boarding_stage);

    // destroy print mutex
    pthread_mutex_destroy(&print_mtx);

    return EXIT_SUCCESS;
}