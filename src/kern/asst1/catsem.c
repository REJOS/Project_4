/*
 * catsem.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use SEMAPHORES to solve the cat syncronization problem in
 * this file.
 */


/*
 *
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>


/*
 *
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2


/*
 *
 * Function Definitions
 *
 */

static struct semaphore *mutex;

static volatile int catsInRoom;
static volatile int miceInRoom;

static volatile int dish_1_busy;
static volatile int dish_2_busy;

static volatile int waiting; // -1 = mouse 1 = cat 0 = no one;

static volatile int catsDone;
static volatile int miceDone;


/*
 * catsem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      If there are mice in the room or if there is a mouse waiting the cat waits and tries to set
 *      waiting to cat waiting. When the cat is done waiting it enters the room, eats, and leaves.
 *
 */

static
void
catsem(void * unusedpointer,
       unsigned long catnumber)
{

        while(miceInRoom>0||waiting==-1){if(waiting==0){waiting=1;}}
        int dish;
        waiting = 0;
         catsInRoom++;
        P(mutex);

            kprintf("Cat %d entered the room\n", catnumber);
            if(dish_1_busy==0){
                dish = 1;
                dish_1_busy = 1;
            }
            else{
                dish = 2;
                dish_2_busy = 1;
            }
            kprintf("Cat %d starting to eat at dish %d\n", catnumber, dish);

            clocksleep(1);
            kprintf("Cat %d finished eating at dish %d\n", catnumber, dish);

            if(dish==1){
                dish_1_busy=0;
            }
            else{
                dish_2_busy=0;
            }
            kprintf("Cat %d leaving the room\n", catnumber);
            catsDone++;
            catsInRoom--;
        V(mutex);


        (void) unusedpointer;
        (void) catnumber;
}


/*
 * mousesem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Checks to see if there are any cats in the room or if there are any cats waiting.
 *      If there are the mice waits for the cat and tries to set the waiting variable to
 *      mouse waiting. When the mouse finishes waiting it enters the room eats and leaves.
 *
 */

static
void
mousesem(void * unusedpointer,
         unsigned long mousenumber)
{
        while(catsInRoom>0||waiting==1){if(waiting==0){waiting=-1;}}
        int dish;
                waiting = 0;
                   miceInRoom++;
                P(mutex);

                    kprintf("Mouse %d entered the room\n", mousenumber);
                    if(dish_1_busy==0){
                        dish = 1;
                        dish_1_busy = 1;
                    }
                    else{
                        dish = 2;
                        dish_2_busy = 1;
                    }
                    kprintf("Mouse %d starting to eat at dish %d\n", mousenumber, dish);

                    clocksleep(1);
                    kprintf("Mouse %d finished eating at dish %d\n", mousenumber, dish);

                    if(dish==1){
                        dish_1_busy=0;
                    }
                    else{
                        dish_2_busy=0;
                    }
                    kprintf("Mouse %d leaving the room\n", mousenumber);

                    miceDone++;
                     miceInRoom--;
                V(mutex);


        (void) unusedpointer;
        (void) mousenumber;
}


/*
 * catmousesem()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catsem() and mousesem() threads.  Change this
 *      code as necessary for your solution.
 */

int
catmousesem(int nargs,
            char ** args)
{
        int index, error;

        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;

        mutex = sem_create("mutex", 2);
        catsInRoom = 0;
        miceInRoom = 0;
        dish_1_busy = 0;
        dish_2_busy = 0;
        waiting = 0;
        catsDone = 0;
        miceDone = 0;

        /*
         * Start NCATS catsem() threads.
         */

        for (index = 0; index < NCATS; index++) {

                error = thread_fork("catsem Thread",
                                    NULL,
                                    index,
                                    catsem,
                                    NULL
                                    );

                /*
                 * panic() on error.
                 */

                if (error) {

                        panic("catsem: thread_fork failed: %s\n",
                              strerror(error)
                              );
                }
        }

        /*
         * Start NMICE mousesem() threads.
         */

        for (index = 0; index < NMICE; index++) {

                error = thread_fork("mousesem Thread",
                                    NULL,
                                    index,
                                    mousesem,
                                    NULL
                                    );

                /*
                 * panic() on error.
                 */

                if (error) {

                        panic("mousesem: thread_fork failed: %s\n",
                              strerror(error)
                              );
                }
        }

        while(catsDone < NCATS || miceDone < NMICE){
        }

        sem_destroy(mutex);

        return 0;
}


/*
 * End of catsem.c
 */
