#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>

/* 
 * This simple default synchronization mechanism allows only vehicle at a time
 * into the intersection.   The intersectionSem is used as a a lock.
 * We use a semaphore rather than a lock so that this code will work even
 * before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
 */
static struct cv *ON;
static struct cv *OS;
static struct cv *OW;
static struct cv *OE;
static struct lock *intersectionLock;
static int carInIntersection;
static int NW;
static int SW;
static int EW;
static int WW;
static int DNow;//0 null, 1 north, 2 east, 3 south, 4 west

/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */
void
intersection_sync_init(void)
{
  /* replace this default implementation with your own implementation */
 
  carInIntersection = 0;
  DNow = 0;
  NW = 0;
  SW = 0;
  EW = 0;
  WW = 0;
  ON = cv_create("ON");
  OS = cv_create("OS");
  OW = cv_create("OW");
  OE = cv_create("OE");
  intersectionLock = lock_create("intersectionLock");
}

/* 
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void
intersection_sync_cleanup(void)
{
  /* replace this default implementation with your own implementation */
  cv_destroy(ON);
  cv_destroy(OS);
  cv_destroy(OW);
  cv_destroy(OE);
  lock_destroy(intersectionLock);
}


/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until it is OK for the vehicle to enter the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle is arriving
 *    * destination: the Direction in which the vehicle is trying to go
 *
 * return value: none
 */

void
intersection_before_entry(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  (void)origin;  /* avoid compiler complaint about unused parameter */
  (void)destination; /* avoid compiler complaint about unused parameter */
  if(origin == north){
    lock_acquire(intersectionLock);
    if(DNow == 0){
       DNow = 1;
    }
    if(DNow != 1){
         NW++;
         cv_wait(ON,intersectionLock);
         NW--;
    }
    carInIntersection++;
    lock_release(intersectionLock);
   }
   else if(origin == south){
      lock_acquire(intersectionLock);
      if(DNow == 0){
       DNow = 3;
      }
      if(DNow != 3){
         SW++;
         cv_wait(OS,intersectionLock);
         SW--;
      }
      carInIntersection++;
      lock_release(intersectionLock);
    }
    else if(origin == west){
      lock_acquire(intersectionLock);
      if(DNow == 0){
        DNow = 4;
      }
            if(DNow != 4){
              WW++;
              cv_wait(OW,intersectionLock);
              WW--;
            }
            carInIntersection++;
            lock_release(intersectionLock);
    }
    else{
      lock_acquire(intersectionLock);
       if(DNow == 0){
        DNow = 2;
      }
       if(DNow != 2){
          EW++;
          cv_wait(OE,intersectionLock);
          EW--;
        }
       carInIntersection++;
       lock_release(intersectionLock);
    }
}


/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 * return value: none
 */

void
intersection_after_exit(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  (void)origin;  /* avoid compiler complaint about unused parameter */
  (void)destination; /* avoid compiler complaint about unused parameter */
  if(origin == north){
           lock_acquire(intersectionLock);
           carInIntersection--;
           if(carInIntersection==0){
             if(EW > 0){
                DNow = 2;
                cv_broadcast(OE,intersectionLock);
             }
             else if(SW > 0){
                DNow = 3;
                cv_broadcast(OS,intersectionLock);
             }
             else if(WW > 0){
                DNow = 4;
                cv_broadcast(OW,intersectionLock);
             }
             else {
                DNow = 0;
             }
           }
           lock_release(intersectionLock);
    }
    else if(origin == east){
		    lock_acquire(intersectionLock);
           carInIntersection--;
           if(carInIntersection==0){
             if(SW > 0){
                DNow = 3;
                cv_broadcast(OS,intersectionLock);
             }
             else if(WW > 0){
                DNow = 4;
                cv_broadcast(OW,intersectionLock);
             }
             else if(NW > 0){
                DNow = 1;
                cv_broadcast(ON,intersectionLock);
             }
             else {
                DNow = 0;
             }
           }
           lock_release(intersectionLock);
    }
    else if(origin == south){
           lock_acquire(intersectionLock);
           carInIntersection--;
           if(carInIntersection==0){
             if(WW > 0){
                DNow = 4;
                cv_broadcast(OW,intersectionLock);
             }
             else if(NW > 0){
                DNow = 1;
                cv_broadcast(ON,intersectionLock);
             }
             else if(EW > 0){
                DNow = 2;
                cv_broadcast(OE,intersectionLock);
             }
             else{
                DNow = 0;
             }
           }
           lock_release(intersectionLock);
    }
    else{
       lock_acquire(intersectionLock);
           carInIntersection--;
           if(carInIntersection==0){
             if(NW > 0){
                DNow = 1;
                cv_broadcast(ON,intersectionLock);
             }
             else if(EW > 0){
                DNow = 2;
                cv_broadcast(OE,intersectionLock);
             }
             else if(SW > 0){
                DNow = 3;
                cv_broadcast(OS,intersectionLock);
             }
             else {
                DNow = 0;
             }
           }
           lock_release(intersectionLock);
    }
}
