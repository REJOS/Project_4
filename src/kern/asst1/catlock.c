/*
 * catlock.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use LOCKS/CV'S to solve the cat syncronization problem in 
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
 * Enum
 *
 */

enum turn {
	NOCATMOUSE,
	CATS,
	MICE
};

/*
 * 
 * Variables
 *
 */

static struct lock *mutex;

/* wait for the cat or mouse turn */
static struct cv *turn_cv;

/* wait here for process done */
static struct cv *done_cv;

/* 1. NOCATMOUSE, 2. CATS 3. MICE */
static volatile int turn_type;

// Dish variables
static volatile int dish1_busy;

static volatile int dish2_busy;

// Cat variables.
static volatile int cats_wait_count;

static volatile int cats_in_this_turn;

static volatile int cats_eat_count;

static volatile int cats_done;

// Mouse variables.
static volatile int mice_wait_count;

static volatile int mice_in_this_turn;

static volatile int mice_eat_count;

static volatile int mice_done;

/*
 *
 * Function Definitions
 * 
 */

/*
 * change_turn()
 *
 * Arguments:
 *      none.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *		Changes the turn when necessary to avoid starvation (every two mice or
 *		cats).
 *
 */

static
void
change_turn()
{
	// Case 1: there are waiting mice
	if (mice_wait_count != 0) {
		turn_type = MICE;
		mice_in_this_turn = 2;
		kprintf("It is mice turn now.\n");
		cv_broadcast(done_cv, mutex);
	} else if (cats_wait_count != 0) { // Case 2: let cats eat
		turn_type = CATS;
		cats_in_this_turn = 2;
		kprintf("It is cats turn now.\n");
		cv_broadcast(turn_cv, mutex);
	} else { // Case 3: no waiting cats or mice
		turn_type = NOCATMOUSE;
	}

}

/*
 * catlock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS -
 *      1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Acquires the lock and checks to see if it is the cats' turn or if it
 *      is no one's turn. If it is the mice's turn or their are already two
 *      cats in the turn, then it waits on on the condition variable turn_cv.
 *      After two cats go, it gives the mice a chance to eat. Increments the
 *      cats_done variable when the cat is done.
 *
 */

static
void
catlock(void * unusedpointer, 
        unsigned long catnumber)
{
	/*
	 * Avoid unused variable warnings.
	 */

	(void) unusedpointer;
	(void) catnumber;

	int mydish;

	lock_acquire(mutex);
	cats_wait_count++; // Initial value = 0

	if (turn_type == NOCATMOUSE) {
		turn_type = CATS;
		cats_in_this_turn = 2; // Two cats per turn
	}

	// Wait until it is the cat turn.
	while (turn_type != CATS || cats_in_this_turn == 0) {
		cv_wait(turn_cv, mutex);
	}

	cats_in_this_turn--; // one cat in the kitchen

	cats_eat_count++; // Initial value = 0
	kprintf("Cat %d enters the kitchen.\n", catnumber);

	if (dish1_busy == 0) {
	    dish1_busy = 1;
	    mydish = 1;
	} else {
	    dish2_busy = 1;
	    mydish = 2;
	}

	kprintf("Cat %d is eating at dish %d.\n", catnumber, mydish); // which cat?

	lock_release(mutex); // release the lock

	clocksleep(1); // enjoys food

	lock_acquire(mutex);

	kprintf("Cat %d finishes eating at dish %d\n", catnumber, mydish);

	if (mydish == 1) /* release dish 1 */
	    dish1_busy = 0;
	else /* release dish 2 */
	    dish2_busy = 0;

	/*update the number of cats in kitchen*/
	cats_eat_count--;

	/*update the number of waiting cats*/
	cats_wait_count--;

	kprintf("Cat %d leaves kitchen.\n", catnumber);

	if (mice_wait_count == 0 && cats_wait_count != 0) {
	    /* Wake up one waiting cat to enter */
	    cats_in_this_turn++;
	    cv_signal(turn_cv, mutex);
	}

	/*Last cat and timeout. Then switch to mice.*/
	if (cats_eat_count == 0) {
	    /*see next slide for detail*/
	    change_turn();
	}

	lock_release(mutex);

	cats_done++;
}
	

/*
 * mouselock()
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
 *      Acquires the lock and checks to see if it is the mice's turn or if it
 *      is no one's turn. If it is the cats' turn or there are already two
 *      mice in the turn, then it waits on on the condition variable done_cv.
 *      After two mice go, it gives the cats a chance to eat. Increments the
 *      mice_done variable when the cat is done.
 *
 */

static
void
mouselock(void * unusedpointer,
          unsigned long mousenumber)
{
	/*
	 * Avoid unused variable warnings.
	 */
        
	(void) unusedpointer;
	(void) mousenumber;

	int mydish;

	lock_acquire(mutex);
	mice_wait_count++; // Initial value = 0

	if (turn_type == NOCATMOUSE) {
		turn_type = MICE;
		mice_in_this_turn = 2; // Two mice per turn
	}

	// Wait until it is the mice turn.
	while (turn_type != MICE || mice_in_this_turn == 0) {
		cv_wait(done_cv, mutex);
	}

	mice_in_this_turn--; // one mouse in the kitchen

	mice_eat_count++; // Initial value = 0
	kprintf("Mouse %d enters the kitchen.\n", mousenumber);

	if (dish1_busy == 0) {
		dish1_busy = 1;
		mydish = 1;
	} else {
		dish2_busy = 1;
		mydish = 2;
	}

	kprintf("Mouse %d is eating at dish %d.\n", mousenumber, mydish); // which mouse?

	lock_release(mutex); // release the lock

	clocksleep(1); // enjoys food

	lock_acquire(mutex);

	kprintf("Mouse %d finishes eating at dish %d\n", mousenumber, mydish);

	if (mydish == 1) { // release dish 1
		dish1_busy = 0;
	} else { // release dish 2
		dish2_busy = 0;
	}

	/*update the number of mice in kitchen*/
	mice_eat_count--;

	/*update the number of waiting mice*/
	mice_wait_count--;

	kprintf("Mouse %d leaves kitchen.\n", mousenumber);

	if (cats_wait_count == 0 && mice_wait_count != 0) {
		/* Wake up one waiting mouse to enter */
		mice_in_this_turn++;
		cv_signal(done_cv, mutex);
	}

	/*Last mouse and timeout. Then switch to mice.*/
	if (mice_eat_count == 0) {
		/*see next slide for detail*/
		change_turn();
	}

	lock_release(mutex);

	mice_done++;
}


/*
 * catmouselock()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catlock() and mouselock() threads. Initalizes
 *      the mutex and the two condition variables, creates the cats and mice
 *      threads, waits for them to complete and then destroys the mutex and
 *      condition variables.
 */

int
catmouselock(int nargs,
             char ** args)
{
        int index, error;
   
        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;

        /*
         * Setup locks and condition variables.
         */

        mutex = lock_create("catlock mutex");
        turn_cv = cv_create("catlock turn cv");
        done_cv = cv_create("catlock done cv");
        turn_type = NOCATMOUSE;
        dish1_busy = 0;
        dish2_busy = 0;
        cats_wait_count = 0;
        cats_in_this_turn = 0;
        cats_eat_count = 0;
        cats_done = 0;
        mice_wait_count = 0;
        mice_in_this_turn = 0;
        mice_eat_count = 0;
        mice_done = 0;
   
        /*
         * Start NCATS catlock() threads.
         */

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catlock thread", 
                                    NULL, 
                                    index, 
                                    catlock, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catlock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        /*
         * Start NMICE mouselock() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mouselock thread", 
                                    NULL, 
                                    index, 
                                    mouselock, 
                                    NULL
                                    );
      
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mouselock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        /*
         * Wait for threads to complete.
         */

        while(cats_done < NCATS || mice_done < NMICE) {
        	clocksleep(1);
        }

        /*
         * Cleanup.
         */

        lock_destroy(mutex);
        cv_destroy(turn_cv);
        cv_destroy(done_cv);

        return 0;
}

/*
 * End of catlock.c
 */
