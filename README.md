# Title: COMP 3430 Operating Systems
## Subtitle: "Assignment 3: Concurrency!"
## Date: Winter 2025
## Author: Het Patel (7972424)

# Contents of the directory

```
[patelh29@eagle cs3430-a3]> tree
.
├── Makefile
├── nqp_list_insertion.c
├── nqp_mlfq_sched.c
├── nqp_mlfq_sched.h
├── nqp_printer.c
├── nqp_refiner.c
├── nqp_sched_tasks.c
├── nqp_thread.c
├── nqp_thread.h
├── nqp_thread_locks.c
├── nqp_thread_locks.h
├── nqp_thread_lock_test.c
├── nqp_thread_sched.h
└── README.md

0 directories, 14 files
```

# How to run the assignment

- Run the command below to compile the assignment files
```
make
```
- The above command will create the following object files:
    1. nqp_thread_locks.o
    2. nqp_thread.o
    3. nqp_mlfq_sched.o
    - **Notes**: 
        - Since nqp_thread.o need nqp_mlfq_sched.o file, if you are using npq_threads to compile a test program, you have to include nqp_mlfq_sched.o as well.
        - To make compilation easier, I have created a variable `NQP_THREAD` in *Makefile*, which contains *both nqp_thread.o* and *nqp_mlfq_sched.o*
        - Another things since *nqp_thread_locks.o* used `nqp_yield()`, if you can compiling a test program which is using the nqp_thread_locks then you have to compile with the `NQP_THREAD` in *Makefile*, **EVEN IF THE TEST PROGRAM DOES NOT USES `nqp_thread`!**

- Follwoing are the executable files that the above command will create along with the object files:
    1. nqp_sched_tasks
    2. nqp_thread_lock_test
    3. nqp_refiner
    4. nqp_printer
    5. nqp_list_insertion

# Test files I(Het) created

## nqp_thread_lock_test.c

- This file test the correctness and mutual exclusion of the locks implemented in `nqp_thread_locks.c` file.
- This program will attempt to run a task which will increament a counter to **100000** by create threads using `pthread_create` and will same 
- The first phase of test will run 2 threads without using any locks and the value of counter after that should be less that 200000 because of race condition in the `for-loop`.
- The first phase of test will run 2 threads using
nqp_thread_locks and the value of counter after that should be exactly equal to 200000 because of the lock being present before race condition in the `for-loop`.

# Implementation of Schedulars

## RR, TWOTHREADS and FIFO

- Implementation of the Round Robin(RR) and First In First Out(FIFO) schedular are in the nqp_thread.c
- TWOTHREADS is just a special case for RR with 2 threads in it. So, the implementation of RR is the implementation of the TWOTHREADS.

## MLFQ

- I have implemented the Multi Level Feedback Queue (MLFQ) in the seprated file named nqp_mlfq_sched.c file which have a header file as well (nqp_mlfq_sched.h).

# Changes to Professor's test files

- None of the test files provided by the professor is changed except **nqp_sched_tasks.c**, I have to change the `settings.mlfq_settings.queue_time_allotment` at line **89** to make the output from the program simillar to **MLFQ**. 
- If the value provided by the professor, that is 125000, is used than it was behaving like a Round Robin. The reason behind is that the queue_time_allotment is high and none of the threads was able to excced that time frame and they were never getting palced to lower priority level, which resulted in a Round Robin behaviour (which is possible if somehow all threads manages to stay in same priority).
- When the `settings.mlfq_settings.queue_time_allotment` value is set to `<= 40000` then the output was more simillar to **MLFQ**.
