#include <stdbool.h>
#include <pthread.h>

/**
 * This structure should be dynamically allocated and passed as
 * an argument to your thread using pthread_create.
 * It should be returned by your thread so it can be freed by
 * the joiner thread.
 */
struct thread_data{
    int wait_to_obtain_ms;
    int wait_to_release_ms;
    pthread_mutex_t* mutex;
    pthread_t* thread;
    bool thread_complete_success;
};

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms);