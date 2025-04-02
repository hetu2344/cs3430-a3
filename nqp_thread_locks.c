#include <stdatomic.h>
#include <stdint.h>
#include "nqp_thread_locks.h"
#include <stdlib.h>



struct NQP_THREAD_MUTEX_T {
    atomic_flag guard;
    uint32_t flag;
    uint32_t init;
    // queue
};

nqp_mutex_t *nqp_thread_mutex_init( void )
{
    nqp_mutex_t *lock;
    if((lock = malloc(sizeof(*lock))) != NULL){
        // lock->guard = ATOMIC_FLAG_INIT;
        atomic_flag_clear(&lock->guard);
        lock->flag = 0;
        lock->init = 1;
        return lock;
    }
    return NULL;
}

int nqp_thread_mutex_lock( nqp_mutex_t *mutex )
{
    (void) mutex;

    // check if the lock is not null
    if(mutex == NULL){
        return -1;
    }

    // check if the lock is initialized of not
    if(mutex->init != 1){
        return -1;
    }

    while(atomic_flag_test_and_set(&mutex->guard))
        ;
    // if(mutex->flag == 0){
    //     mutex->flag = 1;
    //     atomic_flag_clear(&mutex->guard);
    // } else {
    //     // add itself in queue
    //     atomic_flag_clear(&mutex->guard);
    //     // park()/yield() ??
    // }

    return 0;
}

int nqp_thread_mutex_trylock( nqp_mutex_t *mutex )
{
    (void) mutex;

    return -1;
}

int nqp_thread_mutex_unlock( nqp_mutex_t *mutex )
{
    (void) mutex;

    // check if the lock is not null
    if(mutex == NULL){
        return -1;
    }

    // check if the lock is initialized of not
    if(mutex->init != 1){
        return -1;
    }

    atomic_flag_clear(&mutex->guard);

    // while(atomic_flag_test_and_set(&mutex->guard))

    return 0;
}

int nqp_thread_mutex_destroy( nqp_mutex_t *mutex )
{
    (void) mutex;

    if(mutex->init){
        free(mutex);
        return 0;
    }

    return -1;
}
