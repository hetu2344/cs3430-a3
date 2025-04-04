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
#include "nqp_mlfq_sched.h"

static nqp_scheduling_policy system_policy = NQP_SP_TWOTHREADS;
const nqp_sp_settings *policy_settings = NULL;

static nqp_mlfq *mlfq_sched = NULL;

typedef nqp_thread_t* (*nqp_sched_get_thread_func)(void);
typedef void (*nqp_sched_start_func)(void);

typedef struct NQP_SCHEDULAR_T{
    nqp_scheduling_policy sched_policy;
    const nqp_sp_settings* sched_settings;
    nqp_sched_get_thread_func get_next_thread;
    nqp_sched_start_func start_sched;
    nqp_sched_get_thread_func get_curr_thread;
}nqp_sched_t;

static nqp_thread_t *main_thread;
static nqp_thread_t* all_threads[100];
static int add_thread_index = 0;
static int curr_thread_index = -1;

// static struct itimerval timer;
static int timer_flag = 0;
// static int timer_intr_usec = 100;

static nqp_sched_t* schedular;

nqp_thread_t *get_current_thread();
nqp_thread_t *get_mlfq_current_thread();
nqp_thread_t *get_fifo_next_thread();
nqp_thread_t *get_mlfq_next_thread();
nqp_thread_t *get_rr_next_thread();
void handle_nqp_fifo_start();
void handle_nqp_mlfq_start();
void handle_nqp_rr_start();

void timer_interrupt_handler(int sig){
    // printf("interrupt\n");
    (void) sig;
    if(!timer_flag){
        timer_flag = 1;
        nqp_yield();
    // } else {
        // printf("end interrupt\n");
    }
}

nqp_thread_t *nqp_thread_create( void (*task)(void *), void *arg )
{
    assert( task != NULL );
    (void) arg;

    if ( task != NULL )
    {
        // create a stack for the thread to execute
        char *stack = malloc(SIGSTKSZ);
        if(stack == NULL){
            return NULL;
        }
        nqp_thread_t *new_thread = malloc(sizeof(*new_thread));
        if(new_thread == NULL){
            return NULL;
        }
        new_thread->stack = stack;
        new_thread->task = task;
        new_thread->arg = arg;
     
        ucontext_t new_cont;
        // ucontext_t new_cont;
        getcontext(&new_cont);
        new_cont.uc_stack.ss_sp = stack;
        new_cont.uc_stack.ss_size = SIGSTKSZ;
        new_cont.uc_link = NULL;

        if(arg == NULL){
            makecontext(&new_cont, (void (*)(void)) task, 0);
        } else {
            makecontext(&new_cont, (void (*)(void)) task, 1, arg);
        }

        new_thread->context = new_cont;
        new_thread->status = READY;
        new_thread->time_in_queue = 0;
        struct timespec zero = {0};
        new_thread->last_start_time = zero;

        if(system_policy == NQP_SP_MLFQ){
            // add the policy to the mlfq schedule
        } else {
            new_thread->priority = -1;
        }
        all_threads[add_thread_index++] = new_thread;

        return new_thread;

    }

    return NULL;
}

int nqp_thread_join( nqp_thread_t *thread )
{
    assert( thread != NULL );

    if ( thread != NULL )
    {
        while(thread->status != DONE){
            // printf("hiii\n");
            nqp_yield();
        }
    }
    
    return -1;
}

void init_nqp_sched(){
    schedular = malloc(sizeof(*schedular));
    schedular->sched_policy = system_policy;
    schedular->sched_settings = policy_settings;
    if(system_policy == NQP_SP_FIFO){
        schedular->get_next_thread = get_fifo_next_thread;
        schedular->start_sched = handle_nqp_fifo_start;
        schedular->get_curr_thread = get_current_thread;
    } else if(system_policy == NQP_SP_RR || system_policy == NQP_SP_TWOTHREADS){
        schedular->get_next_thread = get_rr_next_thread;
        schedular->start_sched = handle_nqp_rr_start;
        schedular->get_curr_thread = get_current_thread;
    } else if(system_policy == NQP_SP_MLFQ){
        schedular->get_next_thread = get_mlfq_next_thread;
        schedular->start_sched = handle_nqp_mlfq_start;
        schedular->get_curr_thread = get_mlfq_current_thread;
    }
}

int nqp_sched_init( const nqp_scheduling_policy policy,
                    const nqp_sp_settings *settings )
{
    int ret = -1;
    (void) settings;

    assert( policy >= NQP_SP_TWOTHREADS && policy < NQP_SP_POLICIES );

    if ( policy >= NQP_SP_TWOTHREADS && policy < NQP_SP_POLICIES )
    {
        system_policy = policy;
        policy_settings = settings;
        ret = 0;
    }
    return ret;
}

nqp_thread_t *get_rr_next_thread(){
// printf("get_next_thread()\n");
    // if(curr_thread_index == add_thread_index){
    //     curr_thread_index = -1;
    //     return NULL;
    // }

    // curr_thread_index = curr_thread_index == 1 ? 0 : 1;
    // return all_threads[curr_thread_index];
    nqp_thread_t *next_thread;
    int next_thread_index = curr_thread_index;
    int checked = 0;

    if (add_thread_index == 0) {
        return NULL; // No threads
    }

    do {
        next_thread_index = (next_thread_index + 1) % add_thread_index;
        next_thread = all_threads[next_thread_index];
        // printf("next_thread = %d\n", next_thread_index);
        // printf("checked = %d\n", checked);
        
        if (checked >= add_thread_index) {
            // printf("All job finished\n");
            return NULL; // No READY or SLEEP threads
        }
        checked++;
    } while (next_thread == NULL || (next_thread->status != READY && next_thread->status != SLEEP));

    curr_thread_index = next_thread_index;
    return next_thread;
    
}

nqp_thread_t *get_current_thread(){
    return all_threads[curr_thread_index];
}

nqp_thread_t *get_mlfq_current_thread(){
    return get_last_thread(mlfq_sched);
}

nqp_thread_t *get_mlfq_next_thread(){
    return get_next_thread(mlfq_sched);
}

nqp_thread_t *get_fifo_next_thread(){
    // if(curr_thread_index == add_thread_index){
    //     // end of the queue, no job left
    //     return NULL;
    // } else {
    //     return all_threads[++curr_thread_index];
    // }

    nqp_thread_t *ret_thread = all_threads[curr_thread_index];

    if(ret_thread->status == DONE){
        if(curr_thread_index == add_thread_index){
            // end of the queue, no job left
            return NULL;
        } else {
            return all_threads[++curr_thread_index];
        }
    }
    return ret_thread;
}

// nqp_thread_t *get_next_thread(){
//     if(system_policy == NQP_SP_TWOTHREADS || system_policy == NQP_SP_RR){
//         return get_rr_next_thread();
//     } else if(system_policy == NQP_SP_FIFO){
//         return get_fifo_next_thread();
//     }

//     return NULL;
// }

void nqp_yield( void )
{
    if (!add_thread_index){
        return;
    }
    
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    nqp_thread_t* curr_thread = schedular->get_curr_thread();

    // set the priority of main thread to 0 (low priority)
    if(system_policy == NQP_SP_MLFQ && curr_thread == main_thread){
        // printf("\n\nMain Thread\n\n");
        
        int rand_priority = rand() % (get_num_queues(mlfq_sched) - 1);
        curr_thread->priority = rand_priority;
    }

    struct timespec last_start_time = curr_thread->last_start_time;
    useconds_t time_spend = (current_time.tv_sec - last_start_time.tv_sec) * 1000000 + (current_time.tv_nsec - last_start_time.tv_nsec) / 1000;
    curr_thread->time_in_queue += time_spend;
    //printf("Thread[%s]: %d\n", (char *) curr_thread->arg, curr_thread->time_in_queue);

    // schedule another (maybe different) task.
    // if(curr_thread_index < 0 || curr_thread_index >= add_thread_index){
    // // if( curr_thread_index >= add_thread_index){
    //     timer_flag = 0;
    //     return;
    // }

    // update the context of the current running thread

    curr_thread->status = curr_thread->status == DONE ? DONE : SLEEP;
    // swap to the next thread
    nqp_thread_t* next_thread = schedular->get_next_thread();
    if(next_thread == NULL){
        // struct itimerval timer = {0}; // Zeroes both it_value and it_interval
        // if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        //     perror("setitimer disable");
        //     exit(EXIT_FAILURE);
        // }
        setcontext(&main_thread->context);
        timer_flag = 0;
        return;
    }
    // printf("\nThread [Schedular]: swapping context from [%d] to [%d]\n\n", curr_thread_index ? 1 : 2, curr_thread_index ? 2 : 1);
    next_thread->status = RUNNING;
    timer_flag = 0;
    
    clock_gettime(CLOCK_REALTIME, &current_time);
    next_thread->last_start_time = current_time;
    swapcontext(&curr_thread->context, &next_thread->context);
}

void nqp_exit( void )
{
    // remove the currently executing thread from the system.
    nqp_thread_t *curr_thread = schedular->get_curr_thread();
    curr_thread->status = DONE;
    // printf("exit called\n");
    if(!timer_flag){
        timer_flag = 1;
        nqp_yield();
    }
}

// void init_timer_interrupt(){
//     signal(SIGALRM, timer_interrupt_handler);
//     timer.it_value.tv_sec = 0;
//     timer.it_value.tv_usec = timer_intr_usec;

//     timer.it_interval.tv_sec = 0;
//     timer.it_interval.tv_usec = timer_intr_usec;

//     if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
//         perror("setitimer");
//         exit(EXIT_FAILURE);
//     }
// }

void randomly_add_main_thread(){
    // setting up the seed
    srand(time(NULL));
    int r_index = rand() % (add_thread_index + 1);
    // printf("Adding at index: [%d]\n", r_index);

    main_thread = malloc(sizeof(*main_thread));
    main_thread->status = SLEEP;
    main_thread->arg = NULL;
    main_thread->task = NULL;
    // main_thread->context = {0};
    
    // make space for the main thread
    for(int i = add_thread_index - 1; i >= r_index; i--){
        all_threads[i+1] = all_threads[i];
    }
    
    getcontext(&main_thread->context);
    all_threads[r_index] = main_thread;
    add_thread_index++;
}

void handle_nqp_rr_start(){
    // start with first thread: index = 0
    randomly_add_main_thread();
    curr_thread_index = 0;
    // init_timer_interrupt();
    all_threads[0]->status = RUNNING;
    swapcontext(&main_thread->context, &all_threads[0]->context);
}

void handle_nqp_fifo_start(){
    curr_thread_index = 0;

    main_thread = malloc(sizeof(*main_thread));
    main_thread->status = SLEEP;
    main_thread->arg = NULL;
    main_thread->task = NULL;
    // main_thread->context = {0};
    // assigining main context to be the last in the queue
    swapcontext(&main_thread->context, &all_threads[0]->context);
}

void handle_nqp_mlfq_start(){
    randomly_add_main_thread();
    mlfq_sched = init_mlfq_sched(&policy_settings->mlfq_settings, all_threads, (const int) add_thread_index);
    nqp_thread_t *next_thread = get_next_thread(mlfq_sched);
    next_thread->status = RUNNING;

    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    next_thread->last_start_time = current_time;
    swapcontext(&main_thread->context, &next_thread->context);
    return;
}

void nqp_sched_start( void )
{
    // if(system_policy == NQP_SP_TWOTHREADS || system_policy == NQP_SP_RR){
    //     handle_nqp_rr_start();
    // } else if(system_policy == NQP_SP_FIFO){
    //     handle_nqp_fifo_start();
    // }

    // nqp_sched_init() not called by use
    // so default will be set to RR/TWO_THREADS
    // if(schedular == NULL){
    init_nqp_sched();
    // }
    schedular->start_sched();
}
