/*
   Students: Esmeralda Amado, Cristian Hernandez Juan, Gustavo Trejo
   Class: CS 4440 - Operating Systems
   Description: Q2 Mother Hubbard Problem  
*/

// Importing Libraries 
#include <stdio.h>    
#include <stdlib.h>     
#include <limits.h>   
#include <unistd.h>  
#include <pthread.h>  
#include <semaphore.h>  


// Num of Children 
#define NUM_CHILDREN 12

//Number of tasks 
#define NUM_MOTHER_TASKS 5

 // Index of the bath inside motherTaskNames; the bath is the Mother's last task and the Father's work on that child
#define BATH_TASK_INDEX (NUM_MOTHER_TASKS - 1)

// Microseconds the mother pauses after completing one task on one child 
#define MOTHER_TASK_DELAY_USEC 100

 // Microseconds the father pauses after each of his tasks, so that the parallel overlap is visible in the output
#define FATHER_TASK_DELAY_USEC 100


// Mother's tasks in order 
const char *motherTaskNames[NUM_MOTHER_TASKS] = {
    "woken up",
    "fed breakfast",
    "sent to school",
    "given dinner",
    "given a bath"
};

 // Total num of days/cycles, It is written once before the threads start and then only read.
int totalDays = 0;

// Wakes the Mother up. Starts at 1 so she's awake on day 1, and the Father
// posts it when he goes to sleep so she can start the next day.
sem_t motherSemaphore;

// Counts baths the Father hasn't gotten to yet. Starts at 0 so he begins
// asleep, and the Mother posts it after each bath to let him read to that child.
sem_t fatherSemaphore;


 // Thread routine for the Mother, which applies each task in order to every child in order and signals the Father after each bath, then naps until Father wakes her for the next day. 
void *motherRoutine(void *threadArgument){
    (void)threadArgument; 


    // Runs the mothers half of the household once per day, for every day that is inputted. 
    for (int currentDay = 1; currentDay <= totalDays; currentDay++){

         // Mother sleeps until Father wakes her up. On Day 1 returns immediately since the semaphore starts at 1 which means at the start of the program the mother is awake while the father is asleep. 
        sem_wait(&motherSemaphore);

        // Prints day num and that mother is waking up 
        printf("This is day #%d of a day in the life of Mother Hubbard.\n", currentDay);
        printf("Mother is waking up to take care of the children.\n");

        // Outer loop walks the five tasks in order, finishing one task for all children before the next.
        for (int taskIndex = 0; taskIndex < NUM_MOTHER_TASKS; taskIndex++){

            //Inner loop adds the current task to child 1-12 in order
            for (int childNumber = 1; childNumber <= NUM_CHILDREN; childNumber++){
                printf("Child #%d is being %s.\n", childNumber, motherTaskNames[taskIndex]);

                // Delay after each task on a child 
                usleep(MOTHER_TASK_DELAY_USEC);

                 // If the task was completed and happens to be a bath, signals the father so that he can read to the child while the mother moves on to the next child.
                if (taskIndex == BATH_TASK_INDEX){
                    sem_post(&fatherSemaphore);
                }
            }
        }
        // All of the mothers tasks are done and prints message 
        printf("Mother is going to take a nap while Father finishes with the children.\n");
    }
    // Every day is finished so the mother thread ends. 
    pthread_exit(NULL); 
}


// Thread routine for the Father. For every child in order he waits for the bath task to be finished, and reads and tucks the child in bed. Once all 12 children are in bed he goes to sleep and wakes the mother
void *fatherRoutine(void *threadArgument){
    (void)threadArgument; 

    // Runs the father's half of the household once per day for every day that is inputted 
    for (int currentDay = 1; currentDay <= totalDays; currentDay++){
        
        // loops through child 1- 12 in order, finishing his tasks on each child before moving to the next child.
        for (int childNumber = 1; childNumber <= NUM_CHILDREN; childNumber++){
            
            // Blocks until this child has had their bath. Starts at 0 so the Father begins asleep.
            sem_wait(&fatherSemaphore);

             // If the child number is 1, prints that the father is waking up to read to the children.
            if (childNumber == 1){
                printf("Father is waking up to read to the children.\n");
            }

            printf("Child #%d is being read a book.\n", childNumber);
            usleep(FATHER_TASK_DELAY_USEC); /* Short delay so the overlap with the Mother's baths is visible. */

            printf("Child #%d is being tucked in bed.\n", childNumber);
            usleep(FATHER_TASK_DELAY_USEC); /* Short delay after tucking this child in. */
        }

        // Every child is in bed so the day is over, the father goes to sleep and wakes the mother so the next day can begin.
        printf("Father is going to sleep and waking up Mother to take care of the children.\n");
        printf("This is the end of day #%d of a day in the life of Mother Hubbard.\n", currentDay);

        sem_post(&motherSemaphore); // Wakes up the mother for the next day
    }

    // Every day is finished so the father thread ends.
    pthread_exit(NULL); 
}


// Main function that validates the CLI and initializes the semaphores, starts both threads and waits for them to finish to clean up and exit.
int main(int argc, char *argv[]){
    
    // If no number of days is inputted, prints how to use the program and exits. 
    if (argc != 2){
        fprintf(stderr, "Usage: %s <numberOfDays>\n", argv[0]);
        fprintf(stderr, "Example: %s 100\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Points to the first character strtol couldn't convert 
    char *parseEndPointer = NULL; 
    // Day counts read from the command line are parsed as long integers to avoid overflow, then validated to ensure they are positive whole numbers that fit in an int.
    long requestedDays = strtol(argv[1], &parseEndPointer, 10); 

     // If the argument is empty, it contains the characters that are not numbers or isn't a positive number that fits in an int, prints an error message and exits.
    if (argv[1][0] == '\0' || *parseEndPointer != '\0' || requestedDays <= 0 || requestedDays > INT_MAX){
        fprintf(stderr, "Error: number of days must be a positive whole number, but got \"%s\".\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Safe to store now that the value has been validated.
    totalDays = (int)requestedDays;

     // Creates the mothers semaphore with a starting value 1 so she begins awake, if it fails prints an error message and exits.
    if (sem_init(&motherSemaphore, 0, 1) != 0){
        perror("sem_init(motherSemaphore) failed");
        return EXIT_FAILURE;
    }

    // Creates the Father semaphore with a starting value of 0 so he begins asleep, if it fails prints an error message and exits.
    if (sem_init(&fatherSemaphore, 0, 0) != 0){
        perror("sem_init(fatherSemaphore) failed");
        sem_destroy(&motherSemaphore);
        return EXIT_FAILURE;
    }

    pthread_t motherThread; /* Handle used to create and later join the Mother's thread. */
    pthread_t fatherThread; /* Handle used to create and later join the Father's thread. */

    // Launches the mother thread, cleans up and exits if it fails to launch.
    if (pthread_create(&motherThread, NULL, motherRoutine, NULL) != 0){
        perror("pthread_create(motherThread) failed");
        sem_destroy(&motherSemaphore);
        sem_destroy(&fatherSemaphore);
        return EXIT_FAILURE;
    }

    // Launch the Father thread, cleans up and exits if it fails to launch.
    if (pthread_create(&fatherThread, NULL, fatherRoutine, NULL) != 0){
        perror("pthread_create(fatherThread) failed");
        sem_destroy(&motherSemaphore);
        sem_destroy(&fatherSemaphore);
        return EXIT_FAILURE;
    }
    
    // Waits for both threads to finish and cleans up keeps main alive for the entire simulation 
    pthread_join(motherThread, NULL);
    pthread_join(fatherThread, NULL);


    // Both semaphores are destroyed to clean up resources, since they are no longer needed after the threads have finished.
    sem_destroy(&motherSemaphore);
    sem_destroy(&fatherSemaphore);
    
    // Prints a message that the simulation is complete after the days the user inputted. 
    printf("Mother Hubbard simulation complete after %d day(s).\n", totalDays);
    return EXIT_SUCCESS;
}