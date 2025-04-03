#pragma once

#include <ucontext.h>
#include <stdint.h>
#include <unistd.h>

typedef enum NQP_THREAD_STATUS_T{
    RUNNING,
    DONE,
    READY,
    SLEEP
} nqp_thread_stat;

struct NQP_THREAD_T {
    ucontext_t context;
    char *stack;
    void *arg;
    void (*task)(void *);
    nqp_thread_stat status;
    int priority;
    useconds_t time_in_queue;
};

typedef struct NQP_THREAD_T nqp_thread_t;

/**
 * Initialize an nqp_thread_t. 
 *
 * Args:
 *  task: the function that this thread should begin executing in. Must not be
 *        NULL.
 *  arg: the arguments to be passed to the function. May be NULL if the function
 *       does not actually use the arguments passed to it.
 * Return: An initialized thread or NULL on error.
 */
nqp_thread_t *nqp_thread_create( void (*task)(void *), void *arg );

/**
 * Wait for the specified thread to finish. This function will block the caller
 * until the specified thread is finished its current task.
 *
 * Args:
 *  thread: the thread to wait for. Must not be NULL. Must have been previously
 *          initialized.
 * Return: 0 on success (e.g., the thread has exited), -1 on failure.
 */
int nqp_thread_join( nqp_thread_t *thread );


