#pragma once

#include "nqp_thread.h"
#include "nqp_thread_sched.h"

typedef struct NQP_MLFQ_SCHEDULAR nqp_mlfq;
typedef struct NQP_QUEUE nqp_queue;

nqp_mlfq *init_mlfq_sched(const nqp_sp_mlfq_settings *, nqp_thread_t **, const int);
nqp_queue *init_nqp_queue(int);

int is_empty(nqp_queue *);
int is_full(nqp_queue *);
void increase_size(nqp_queue *);
void enqueue_thread(nqp_queue *, nqp_thread_t *);
nqp_thread_t *dequeue_thread(nqp_queue *);
nqp_thread_t *get_next_thread(nqp_mlfq *);
void apply_mlfq_rule(nqp_mlfq *);
void add_new_thread(nqp_mlfq *, nqp_thread_t *);
