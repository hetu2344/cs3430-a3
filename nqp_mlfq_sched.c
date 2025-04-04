#include "nqp_mlfq_sched.h"

#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <ucontext.h>
#include <stdio.h>
// #include <sys/time.h>
#include <unistd.h>
#include <time.h>

#include "nqp_thread.h"
#include "nqp_thread_sched.h"

struct NQP_MLFQ_SCHEDULAR {
    useconds_t queue_time_allotment; // this is time allotment as described in 
                                     // OSTEP for the final set of MLFQ rules.

    useconds_t boost_time;           // this is S as desribed in OSTEP for the
                                     // final set of MLFQ rules.

    uint8_t queues;                  // how many queues should we have?
    int num_threads;
    nqp_thread_t *last_thread;
    struct timespec last_boost_time;
    nqp_queue *queue_list[];
};

struct NQP_QUEUE{
    int priority;
    int size;
    int enqueue_index;
    int dequeue_index;
    nqp_thread_t **elements;
};

nqp_mlfq *init_mlfq_sched(const nqp_sp_mlfq_settings *settings, nqp_thread_t **all_threads, const int num_threads){
    nqp_mlfq *sched = malloc(sizeof(*sched) + (sizeof(nqp_queue *) * settings->queues));
    sched->queue_time_allotment = settings->queue_time_allotment;
    sched->boost_time = settings->boost_time;
    sched->queues = settings->queues;
    sched->num_threads = num_threads;
    sched->last_thread = NULL;
    for(int i = 0; i < settings->queues; i++){
        nqp_queue *queue = init_nqp_queue(num_threads);
        queue->priority = i;
    }

    // When we are starting the schedular
    // we put all of threads in highest priority queue
    int hpq_index = settings->queues - 1; // Index of Highest Priority Queue in the schedular
    nqp_queue *hp_queue = sched->queue_list[hpq_index]; // Highest Priority queue in the schedular
    for(int i = 0; i < num_threads; i++){
        all_threads[i]->priority = hpq_index;
        enqueue_thread(hp_queue, all_threads[i]);
    }

    return sched;
}

nqp_queue *init_nqp_queue(int size){
    nqp_queue *queue = malloc(sizeof(*queue));
    queue->size = size;
    queue->enqueue_index = 0;
    queue->dequeue_index = 0;
    
    nqp_thread_t **elements = malloc(sizeof(*elements) * size);
    queue->elements = elements;

    return queue;
}

int is_empty(nqp_queue *queue){
    return queue->enqueue_index == queue->dequeue_index;
}

int is_full(nqp_queue *queue){
    return (queue->enqueue_index + 1) % queue->size == queue->dequeue_index;
}

void increase_size(nqp_queue *queue){
    nqp_thread_t **new_elements = malloc(queue->size * 2 * sizeof(*new_elements));

    nqp_thread_t *thread;
    int enqueue_index = 0;
    while((thread = dequeue_thread(queue)) != NULL){
        new_elements[enqueue_index++] = thread;
    }

    // free(queue->elements);
    queue->elements = new_elements;
    queue->enqueue_index = enqueue_index;
    queue->dequeue_index = 0;
    queue->size = queue->size * 2;
}

void enqueue_thread(nqp_queue *queue, nqp_thread_t *new_thread){
    if(is_full(queue)){
        increase_size(queue);
    }

    // add at the enqueue index
    queue->elements[queue->enqueue_index] = new_thread;
    queue->enqueue_index = (queue->enqueue_index + 1) % queue->size;
}

nqp_thread_t *dequeue_thread(nqp_queue *queue){
    nqp_thread_t *thread = NULL;
    if(!is_empty(queue)){
        thread = queue->elements[queue->dequeue_index];
        queue->dequeue_index = (queue->dequeue_index + 1) % queue->size;
    }

    return thread;
}

void apply_mlfq_rules(nqp_mlfq *sched){

    nqp_thread_t *last_thread = sched->last_thread;
    
    if (last_thread != NULL && last_thread->status != DONE) {
        // Rule 4 check
        // Check if the thread exceeded its time allotment and is eligible for demotion
        if (last_thread->time_in_queue >= sched->queue_time_allotment && last_thread->priority > 0) {
            // Demote the thread by lowering its priority
            last_thread->priority--;
            // Optionally, reset the time counter if your design requires it:
            last_thread->time_in_queue = 0;
        }

        // Rule 2 check
        // if last thread's priority is not changed,
        // meaning, I am adding the thread back to the same queue
        // this will make sure that thread is run in Round Robin
        // with other threads of same priority
        // Enqueue the thread back into the appropriate queue
        enqueue_thread(sched->queue_list[last_thread->priority], last_thread);
    }

    // Rule 5 implementation
    // After S time period, move all
    // jobs to topmost priority
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    useconds_t boost_time_diff = (current_time.tv_sec - sched->last_boost_time.tv_sec) * 1000000 + (current_time.tv_nsec - sched->last_boost_time.tv_nsec) / 1000;
    if(boost_time_diff > sched->boost_time){
        // dequeue all threads from all threads except topmost
        // enqueue all queue in the topmost queue
        int queue_to_itir = sched->queues - 2;
        int hpq_index = sched->queues - 1;
        nqp_thread_t *temp;
        for(int i = queue_to_itir; i > -1; i--){
            while((temp = dequeue_thread(sched->queue_list[i])) != NULL){
                enqueue_thread(sched->queue_list[hpq_index], temp);
                temp->priority = hpq_index;
                temp->time_in_queue = 0;
            }
        }
        clock_gettime(CLOCK_REALTIME, &current_time);
        sched->last_boost_time = current_time;
    }
}

// Implementation of Rule 3
// Whenever new thread is arrived in system
// it is placed at topmost priority
void add_new_thread(nqp_mlfq *sched, nqp_thread_t *thread){
    int hpq_index = sched->queues - 1;
    thread->priority = hpq_index;
    enqueue_thread(sched->queue_list[hpq_index], thread);
}

nqp_thread_t *get_next_thread(nqp_mlfq *sched){
    nqp_thread_t *thread = NULL;

    // Beforing deciding which thread to run next
    // lets apply the rule to the all of the 
    // threads in the system
    apply_mlfq_rules(sched);


    // Implementation of Rule 1
    // That run the job with highest priority
    // This loop will iterate through each queue
    // untill a job is not returned from that queue
    // and since the loop is iterating from the end of the 
    // queue_list the highest priority threads are getting 
    // choosen first.
    for(int i = sched->queues - 1; i > -1 && thread == NULL; i--){
        // try get a thread from current queue level
        thread = dequeue_thread(sched->queue_list[i]);
    }
    return thread;
}
