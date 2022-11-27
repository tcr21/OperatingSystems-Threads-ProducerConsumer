/******************************************************************
 The Main program takes in command line arguments, sets up required data
 structurs, creates and initialises semaphores, and creates producers and
 consumers
 ******************************************************************/ 

# include "helper.h" 

// Declare producer and consumer functions
void *producer (void *parameter); 
void *consumer (void *parameter);

// I. MAIN

int main (int argc, char **argv)
{
  // 1. CHECK & TAKE IN COMMAND LINE ARGUMENTS

  // a. Check number of arguments
  if (argc != 5)
  {
    cerr << "ERROR: invalid number of arguments. Please enter 5 arguments including executable name at command line. \n"; 
    return -1; 
  }

  // b. Check each argument is valid
  for (int i = 1; i < argc; i++)
  {
    if (check_arg(argv[i]) < 0)
    {
      return check_arg(argv[i]);
    }
  }
  
  // c. Initialise variables with command line arguments
  int queue_size = stoi(argv[1]);  
  int jobs_per_producer_count = stoi(argv[2]); 
  int producer_count = stoi(argv[3]); 
  int consumer_count = stoi(argv[4]); 

  // 2. SET UP DATA STRUCTURES
  
  // a. Create array of jobs for queue
  Job** job_array = new Job*[queue_size]; 
  for (int i = 0; i < queue_size; i++)
  {
    job_array[i] = nullptr; 
  }  

  // b. Create and initialise queue
  Circular_queue* queue = new Circular_queue
  {
    queue_size,
    -1, // front
    -1, // rear
    job_array, 
  };  

  // 3. CREATE & INITIALISE SEMAPHORES

  // a. Create semaphore array
  key_t sem_key = SEM_KEY; 
  int sem_id = sem_create(sem_key, 3); 
  if (sem_id < 0)
  {
    cerr << "ERROR creating semaphore array. \n"; 
    return sem_id;
  }

  // b. Initialise mutual exclusion
  if (sem_init (sem_id, 0, 1) < 0)
  {
    cerr << "ERROR initialising mutual exclusion semaphore. \n"; 
    return sem_init (sem_id, 0, 1); 
  }
  // c. Initialise check buffer has space
  if (sem_init (sem_id, 1, queue_size) < 0)
  {
    cerr << "ERROR initialising check for space semaphore. \n"; 
    return sem_init (sem_id, 1, queue_size); 
  }
  // d. Initialise check buffer has a job
  if (sem_init (sem_id, 2, 0) < 0)
  {
    cerr << "ERROR initialising check for item semaphore. \n"; 
    return sem_init (sem_id, 2, 0); 
  }
  
  // 4. CREATE PRODUCER & CONSUMER THREADS

  // a. Create producer parameters and threads
  pthread_t** producer_array = new pthread_t*[producer_count]; 
  Producer_parameters** producer_parameters_array = new Producer_parameters*[producer_count]; 
  for (int i = 0; i < producer_count; i++)
  {
    // i. Initialise arrays
    producer_array[i] = new pthread_t; 
    producer_parameters_array[i] = new Producer_parameters 
    {
      i+1,
      jobs_per_producer_count, 
      sem_id,
      queue,
    };

    // ii. Create threads
    if (pthread_create (producer_array[i], NULL, producer, producer_parameters_array[i]) != 0 )
    {
      cerr << "ERROR creating producer thread " << i << endl;
      pthread_exit(0); 
    };
  }

  // b. Create consumer parameters and threads
  pthread_t** consumer_array = new pthread_t*[consumer_count]; 
  Consumer_parameters** consumer_parameters_array = new Consumer_parameters*[consumer_count]; 
  for (int i = 0; i < consumer_count; i++)
  {
    // i. Initialise arrays
    consumer_array[i] = new pthread_t; 
    consumer_parameters_array[i] = new Consumer_parameters 
    {
      i+1, 
      sem_id, 
      queue,
    }; 

    // ii. Create threads
    if (pthread_create (consumer_array[i], NULL, consumer, consumer_parameters_array[i]) != 0)
    {
      cerr << "ERROR creating consumer thread " << i << endl;
      pthread_exit(0); 
    };
  }

  // c. Join producer and consumer threads
  for (int i = 0; i < producer_count; i++)
  {
    if (pthread_join(*producer_array[i], NULL) != 0)
    {
      cerr << "ERROR joining producer thread " << i << endl;
      pthread_exit(0); 
    }; 
  }

  for (int i = 0; i < consumer_count; i++)
  {
    if (pthread_join(*consumer_array[i], NULL) != 0)
    {
      cerr << "ERROR joining producer thread " << i << endl;
      pthread_exit(0); 
    };
  }

  // 5. CLEAN & EXIT

  // a. Clean up heap
  delete [] producer_array; 
  delete [] producer_parameters_array; 
  delete [] consumer_array;
  delete [] consumer_parameters_array; 

  // b. Clean up semaphores
  sem_close(sem_id); 

  return 0;
}

// II. DEFINE PRODUCER FUNCTION (argument in producer thread creation)
void *producer (void *parameter) 
{
  // 1. INITIALISE PARAMETERS
  Producer_parameters *producer_parameters = (Producer_parameters *) parameter;
  int producer_id = producer_parameters->id; 
  int producer_job_count = producer_parameters->job_count; 
  int shared_sem_id = producer_parameters->sem_id; 
  Circular_queue* shared_queue = producer_parameters->queue;
  Job* job = nullptr; 


  // 2. ADD TO QUEUE WHILE THERE ARE STILL JOBS TO PRODUCE
  while (producer_job_count > 0)
  {
    // a. Produce job
    job = new Job;

    // b. If queue is full, wait 20 sec for spot to free up
    if (sem_timed_wait(shared_sem_id, 1, 20) != 0)
    {
      printf("Producer (%i): : No more space in queue \n", producer_id);
      pthread_exit(0);
    }
    // c. Ensure mutual exclusion
    sem_wait(shared_sem_id, 0); 
    
    // d. Deposit job, decrement job count & print status
    shared_queue->add_job(job); 
    producer_job_count--; 
    printf("Producer(%i): Job id %i duration %i \n", producer_id, job->id, job->duration);

    // e. Release mutual exclusion & let consumer know there is an job
    sem_signal(shared_sem_id, 0); 
    sem_signal(shared_sem_id, 2);

    // f. Job only added every 1-5 seconds 
    sleep((rand() % 5) + 1);
  }

  // 3. EXIT WHEN ALL JOBS HAVE BEEN GENERATED
  printf("Producer(%i): No more jobs to generate. \n", producer_id); 
  pthread_exit(0); 
}


// III. DEFINE CONSUMER FUNCTION (argument in consumer thread creation)
void *consumer (void *parameter) 
{
  // 1. INITIALISE PARAMETERS
  Consumer_parameters *consumer_parameters = (Consumer_parameters *) parameter;
  int consumer_id = consumer_parameters->id; 
  int shared_sem_id = consumer_parameters->sem_id; 
  Circular_queue *shared_queue = consumer_parameters->queue;

  // 2. CREATE JOB VARIABLE TO ACCESS JOB TO BE EXTRACTED FROM QUEUE
  Job* job = nullptr; 

  // 3. WHILE POSSIBLE, CONSUME JOBS FROM QUEUE
  while (1)
  {
    // a. Check for job and if there is no job after 20 sec, exit
    if (sem_timed_wait(shared_sem_id, 2, 20) != 0)
    {
      printf("Consumer(%i): No more jobs left \n", consumer_id);
      pthread_exit(0);
    }
    // b. Ensure mutual exclusion
    sem_wait(shared_sem_id, 0);

    // c. Fetch job & print status
    job = shared_queue->remove_job(); 
    printf("Consumer(%i): Job id %i executing sleep duration %i \n", consumer_id, job->id, job->duration); 

    // d. Release mutual exclusion & let producer know there is space
    sem_signal(shared_sem_id, 0);
    sem_signal(shared_sem_id, 1); 

    // e. Consume job & output completion status
    sleep (job->duration); 
    printf("Consumer(%i): Job id %i completed \n", consumer_id, job->id); 

    // f. Clean up heap
    delete job;
    job = nullptr;
  }

  // 4. REDUNDANT GIVEN TIMER
  pthread_exit(0); 
}


