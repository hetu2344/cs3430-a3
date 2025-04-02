#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <ucontext.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "nqp_thread.h"
#include "nqp_thread_sched.h"

static nqp_scheduling_policy system_policy = NQP_SP_TWOTHREADS;

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
};

static ucontext_t main_context;
static nqp_thread_t* all_threads[100];
static int add_thread_index = 0;
static int curr_thread_index = -1;

static struct itimerval timer;
static int timer_flag = 0;
static int timer_intr_usec = 100;

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

    }
    
    return -1;
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

nqp_thread_t *get_fifo_next_thread(){
    if(curr_thread_index == add_thread_index){
        // end of the queue, no job left
        return NULL;
    } else {
        return all_threads[++curr_thread_index];
    }
}

nqp_thread_t *get_next_thread(){
    if(system_policy == NQP_SP_TWOTHREADS || system_policy == NQP_SP_RR){
        return get_rr_next_thread();
    } else if(system_policy == NQP_SP_FIFO){
        return get_fifo_next_thread();
    }

    return NULL;
}

void nqp_yield( void )
{
    // schedule another (maybe different) task.
    if(curr_thread_index < 0 || curr_thread_index >= add_thread_index){
    // if( curr_thread_index >= add_thread_index){
        timer_flag = 0;
        return;
    }

    // update the context of the current running thread
    nqp_thread_t* curr_thread = all_threads[curr_thread_index];

    curr_thread->status = curr_thread->status == DONE ? DONE : SLEEP;
    // swap to the next thread
    nqp_thread_t* next_thread = get_next_thread();
    if(next_thread == NULL){
        struct itimerval timer = {0}; // Zeroes both it_value and it_interval
        if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
            perror("setitimer disable");
            exit(EXIT_FAILURE);
        }
        setcontext(&main_context);
        timer_flag = 0;
        return;
    }
    // printf("\nThread [Schedular]: swapping context from [%d] to [%d]\n\n", curr_thread_index ? 1 : 2, curr_thread_index ? 2 : 1);
    next_thread->status = RUNNING;
    timer_flag = 0;
    swapcontext(&curr_thread->context, &next_thread->context);
}

void nqp_exit( void )
{
    // remove the currently executing thread from the system.
    nqp_thread_t *curr_thread = all_threads[curr_thread_index];
    curr_thread->status = DONE;
    // printf("exit called\n");
    if(!timer_flag){
        timer_flag = 1;
        nqp_yield();
    }
}

void init_timer_interrupt(){
    signal(SIGALRM, timer_interrupt_handler);
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = timer_intr_usec;

    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = timer_intr_usec;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        exit(EXIT_FAILURE);
    }
}

void handle_nqp_rr_start(){
    // start with first thread: index = 0
    curr_thread_index = 0;
    init_timer_interrupt();
    all_threads[0]->status = RUNNING;
    swapcontext(&main_context, &all_threads[0]->context);
}

void handle_nqp_fifo_start(){
    curr_thread_index = 0;
    // assigining main context to be the last in the queue
    swapcontext(&main_context, &all_threads[0]->context);
}

void nqp_sched_start( void )
{
    if(system_policy == NQP_SP_TWOTHREADS || system_policy == NQP_SP_RR){
        handle_nqp_rr_start();
    } else if(system_policy == NQP_SP_FIFO){
        handle_nqp_fifo_start();
    }
}
