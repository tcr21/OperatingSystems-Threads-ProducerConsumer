/******************************************************************
 * Header file for the helper functions. This file includes the
 * required header files, as well as the function signatures and
 * the semaphore values (which are to be changed as needed).
 ******************************************************************/ 


# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/ipc.h>
# include <sys/shm.h>
# include <sys/sem.h>
# include <sys/time.h>
# include <math.h>
# include <errno.h>
# include <string.h>
# include <pthread.h>
# include <ctype.h>
# include <iostream> 
using namespace std;

# define SEM_KEY 0x89 // Changed from 50 to 89

union semun {
    int val;               /* used for SETVAL only */
    struct semid_ds *buf;  /* used for IPC_STAT and IPC_SET */
    ushort *array;         /* used for GETALL and SETALL */
};

int check_arg (char *);
int sem_create (key_t, int);
int sem_init (int, int, int);
void sem_wait (int, short unsigned int);
void sem_signal (int, short unsigned int);
int sem_close (int);

// ADDITIONAL

// I. DATA STRUCTURES

// 1. Job
struct Job
{
    // Data members
    int id;
    int duration; 
}; 

// 2. Circular queue
struct Circular_queue
{
    // Data members
    int queue_size; 
    int front; 
    int rear; 
    Job** job_array; 

    // Add job function (used by producer, increments the rear)
    void add_job(Job* job)
    {
        // For first job to be added
        if (front == -1)
        {
            front = 0;
        }
        // Increment rear
        rear = (rear + 1) % queue_size;
        // Set job id/ duration & deposit job
        job->id = rear;
        job->duration = (rand() % 10) + 1;
        job_array[rear] = job; 
    }

    // Remove job function (used by consumer, increments the front)
    Job* remove_job()
    {
        // Fetch job & release spot in queue
        Job* job = job_array[front]; 
        job_array[front] = nullptr; 
        // For last job to be removed
        if (front == rear)
        {
            rear = -1;
            front = -1;
        }
        // Increment front
        else
        {
            front = (front + 1) % queue_size;
        }
        // Return job
        return job; 
    }  
}; 

// 3. Producer parameters (to be passed as argument in thread creation)
struct Producer_parameters
{
    // Data members
    int id;
    int job_count; 
    int sem_id; 
    Circular_queue* queue;
}; 

// 4. Consumer parameters (to be passed as argument in thread creation)
struct Consumer_parameters
{
    // Data members
    int id;
    int sem_id; 
    Circular_queue* queue; 
}; 

// II. FUNCTIONS

// 1. 20 second timer semaphore function
int sem_timed_wait(int id, short unsigned int num, time_t seconds); 











