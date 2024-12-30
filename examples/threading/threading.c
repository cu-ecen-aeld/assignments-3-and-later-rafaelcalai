#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include<unistd.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    usleep(1000 * thread_func_args->wait_to_obtain_ms);

    pthread_mutex_lock(thread_func_args->mutex);
    usleep(1000 * thread_func_args->wait_to_release_ms);
    pthread_mutex_unlock(thread_func_args->mutex);
    thread_func_args->thread_complete_success = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data* thread_param = (struct thread_data*) malloc(sizeof(struct thread_data));
    if (thread_param == NULL) {
        return 1;
    }

    thread_param->mutex = mutex;
    thread_param->thread = thread;
    thread_param->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_param->wait_to_release_ms = wait_to_release_ms;
    thread_param->thread_complete_success = false;

    int status = pthread_create( thread, NULL, threadfunc, (void*) thread_param);

    return true ? status == 0 : false;
}