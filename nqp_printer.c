#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include "nqp_thread.h"
#include "nqp_thread_sched.h"

void printer( void *arg )
{
    assert( arg != NULL );

    uint64_t counter = 0;
    char letter = '\0';

    if ( arg != NULL )
    {
        letter = *(char *)arg;
        while ( counter < 20 )
        {
            printf("%c %lu\n", letter, counter );
            counter++;

            if ( counter % 5 == 0  && counter != 20)
            {
                nqp_yield( );
            }
        }
    }

    nqp_exit( );
}

int main( void )
{
    char *letters = "abcdefg";
    
    nqp_thread_t *thread_a = nqp_thread_create( printer, &letters[0] );
    nqp_thread_t *thread_b = nqp_thread_create( printer, &letters[1] );
    nqp_thread_t *thread_c = nqp_thread_create( printer, &letters[2] );
    nqp_thread_t *thread_d = nqp_thread_create( printer, &letters[3] );
    nqp_thread_t *thread_e = nqp_thread_create( printer, &letters[4] );
    nqp_thread_t *thread_f = nqp_thread_create( printer, &letters[5] );
    nqp_thread_t *thread_g = nqp_thread_create( printer, &letters[6] );

    // nqp_sched_init(NQP_SP_FIFO, NULL);
    nqp_sched_start();
    // (void) thread_a;
    // (void) thread_b;


    printf("Thread [main]: Waiting for Thread [a]\n");
    nqp_thread_join( thread_a );
    printf("Thread [main]: Joining Thread [a]\n");
    
    printf("Thread [main]: Waiting for Thread [b]\n");
    nqp_thread_join( thread_b );
    printf("Thread [main]: Joining Thread [b]\n");
    
    printf("Thread [main]: Waiting for Thread [c]\n");
    nqp_thread_join( thread_c );
    printf("Thread [main]: Joining Thread [c]\n");
    
    printf("Thread [main]: Waiting for Thread [d]\n");
    nqp_thread_join( thread_d );
    printf("Thread [main]: Joining Thread [d]\n");
    
    printf("Thread [main]: Waiting for Thread [e]\n");
    nqp_thread_join( thread_e );
    printf("Thread [main]: Joining Thread [e]\n");
    
    printf("Thread [main]: Waiting for Thread [f]\n");
    nqp_thread_join( thread_f );
    printf("Thread [main]: Joining Thread [f]\n");
    
    printf("Thread [main]: Waiting for Thread [g]\n");
    nqp_thread_join( thread_g );
    printf("Thread [main]: Joining Thread [g]\n");

    printf("\nThread [main]: End of program\n\n");
    return EXIT_SUCCESS;
}
