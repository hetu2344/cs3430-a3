# COMP 3430 Operating Systems
# Winter 2025
# Franklin Bristow
#
# Students registered in this offering of the course are explicitly permitted
# to copy and use this Makefile for their own work.

CC = clang
CFLAGS = -Wall -Werror -Wextra -Wpedantic  -D_FORTIFY_SOURCE=3 -g
NQP_THREAD = nqp_thread.o nqp_mlfq_sched.o # .o files needed to comple and use nqp_thread.c

.PHONY: clean

all: nqp_printer nqp_refiner nqp_sched_tasks nqp_list_insertion nqp_thread_lock_test nqp_2_thread_test

# nqp_mlfq_sched.o:
# 	clang -Wall -Werror -Wextra -Wpedantic -D_FORTIFY_SOURCE=3 -g -c nqp_mlfq_sched.c -o nqp_mlfq_sched.o

nqp_thread_lock_test: nqp_thread_locks.o $(NQP_THREAD)

nqp_2_thread_test: $(NQP_THREAD) nqp_thread_locks.o 

nqp_printer: $(NQP_THREAD) 

nqp_refiner: $(NQP_THREAD) 

nqp_sched_tasks: $(NQP_THREAD) 

nqp_list_insertion: $(NQP_THREAD) nqp_thread_locks.o 

clean:
	rm -rf *.o nqp_printer nqp_refiner \
		nqp_sched_tasks nqp_list_insertion \
		nqp_2_thread_test nqp_thread_lock_test
