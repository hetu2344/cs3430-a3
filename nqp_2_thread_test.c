#include <stdlib.h>
#include <stdio.h>

#include "nqp_thread_locks.h"
#include "nqp_thread.h"
#include "nqp_thread_sched.h"

int counter = 0;
nqp_mutex_t *lock;

void increment_counter_to_100000(void *args){
    printf("\nThread [%d]: Started\n",  *(int *)args);

    for(int i = 0; i < 100000; i++){
        // if(i == 42352 && *(int *)args == 1) {
        //     printf("Thread [%d]: Before nqp_yield() for-counter is %d\n", *(int *)args, i);
        //     nqp_yield();
        //     printf("Thread [%d]: After nqp_yield() for-counter is %d\n", *(int *)args, i);
        // }
        // if(i == 72632 && *(int *)args == 2) {
        //     printf("Thread [%d]: Before nqp_yield() for-counter is %d\n", *(int *)args, i);
        //     nqp_yield();
        //     printf("Thread [%d]: After nqp_yield() for-counter is %d\n", *(int *)args, i);
        // }
        nqp_thread_mutex_lock(lock);
        counter++;
        nqp_thread_mutex_unlock(lock);
    }
    // while(1)
    //     ;
    // printf("Thread [%d]: Counter is %d\n", *(int *)args, counter);
    printf("Thread [%d]: Ended\n\n", *(int *) args);
    // nqp_yield();
    nqp_exit();
    // return NULL;
}

int main(void){
    int thread_ids[] = {1,2,3,4,5};
    
    lock = nqp_thread_mutex_init();

    if(lock == NULL){
        printf("Error creating the lock\n");
        exit(-1);
    }

    printf("\nThread [main]: Counter is %d\n\n", counter);
    // printf("Starting thread 1\n");
    nqp_thread_t *t1 = nqp_thread_create(increment_counter_to_100000, &thread_ids[0]);
    nqp_thread_t *t2 = nqp_thread_create(increment_counter_to_100000, &thread_ids[1]);
    nqp_thread_t *t3 = nqp_thread_create(increment_counter_to_100000, &thread_ids[2]);
    nqp_thread_t *t4 = nqp_thread_create(increment_counter_to_100000, &thread_ids[3]);
    nqp_thread_t *t5 = nqp_thread_create(increment_counter_to_100000, &thread_ids[4]);
    (void) t1;
    (void) t2;
    (void) t3;
    (void) t4;
    (void) t5;
    // pthread_join(thread_1, NULL);
    // printf("Thread [1]: Returned.\n");

    // pthread_join(thread_2, NULL);
    // printf("Thread [2]: Returned\n");

    // Comment out the line below to run in RR or default policy
    nqp_sched_init(NQP_SP_FIFO, NULL);

    nqp_sched_start();

    printf("\nThread [main]: Counter is %d\n", counter);

    printf("\nEnd of Test\n");

    return 0;
}
