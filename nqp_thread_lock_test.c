#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "nqp_thread_locks.h"

int counter = 0;
nqp_mutex_t *lock;

void *increment_counter_to_100000(void *args){
    printf("Thread [%d]: Started\n",  *(int *)args);

    for(int i = 0; i < 100000; i++){
        nqp_thread_mutex_lock(lock);
        counter++;
        nqp_thread_mutex_unlock(lock);
    }

    printf("Thread [%d]: Ended\n", *(int *) args);

    return NULL;
}

int main(void){
    pthread_t thread_1;
    pthread_t thread_2;
    int thread_ids[] = {1,2};
    
    lock = nqp_thread_mutex_init();

    if(lock == NULL){
        printf("Error creating the lock\n");
        exit(-1);
    }

    printf("\nThread [main]: Counter is %d\n", counter);
    // printf("Starting thread 1\n");
    pthread_create(&thread_1, NULL, increment_counter_to_100000,  &thread_ids[0]);
    pthread_create(&thread_2, NULL, increment_counter_to_100000,  &thread_ids[1]);

    pthread_join(thread_1, NULL);
    printf("Thread [1]: Returned.\n");

    pthread_join(thread_2, NULL);
    printf("Thread [2]: Returned\n");

    printf("\nThread [main]: Counter is %d\n", counter);

    printf("\nEnd of Test\n");

    return 0;
}
