#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>
#include <wchan.h>

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
static struct cv *NS;
static struct cv *NS_L;
static struct cv *WE;
static struct cv *WE_L;
static struct lock *intersectionLock;
// static struct lock *NS_Lock;
// static struct lock *NSL_Lock;
// static struct lock *WE_Lock;
// static struct lock *WEL_Lock;
static int CarInIntersection;


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

  CarInIntersection = 0;
  NS = cv_create("NS");
  NS_L = cv_create("NS_L");
  WE = cv_create("WE");
  WE_L = cv_create("WE_L");
  intersectionLock = lock_create("intersectionLock");
  // NS_Lock = lock_create("NS_Lock");
  // NSL_Lock = lock_create("NSL_Lock");
  // WE_Lock = lock_create("WE_Lock");
  // WEL_Lock = lock_create("WEL_Lock");
  return;
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
  cv_destroy(NS);
  cv_destroy(NS_L);
  cv_destroy(WE);
  cv_destroy(WE_L);
  lock_destroy(intersectionLock);
  // lock_destroy(NS_Lock);
  // lock_destroy(NSL_Lock);
  // lock_destroy(WE_Lock);
  // lock_destroy(WEL_Lock);
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
  if(CarInIntersection >0){
    if((origin == north && destination == south) || (origin == south && destination == north) || 
        (origin == north && destination == west) || (origin == south && destination == east)){
		      cv_wait(NS,intersectionLock);
          CarInIntersection++;
          lock_release(intersectionLock);
    }
    else if((origin == north && destination == east) || (origin == south && destination == west)){
		      cv_wait(NS_L,intersectionLock);
          CarInIntersection++;
          lock_release(intersectionLock);
    }
    else if((origin == west && destination == east) || (origin == east && destination == west) ||
         (origin == west && destination == south) || (origin == east && destination == north)){
            cv_wait(WE,intersectionLock);
            CarInIntersection++;
            lock_release(intersectionLock);
    }
    else{
       cv_wait(WE_L,intersectionLock);
       CarInIntersection++;
       lock_release(intersectionLock);
    }
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
  if((origin == north && destination == south) || (origin == south && destination == north) || 
        (origin == north && destination == west) || (origin == south && destination == east)){
		       lock_acquire(intersectionLock);
           CarInIntersection--;
           if(CarInIntersection==0){
             if(!wchan_isempty(WE_L->cv_wchan)){
                cv_broadcast(WE_L,intersectionLock);
             }
             else if(!wchan_isempty(WE->cv_wchan)){
                cv_broadcast(WE,intersectionLock);
             }
             else if(!wchan_isempty(NS_L->cv_wchan)){
                cv_broadcast(NS_L,intersectionLock);
             }
		         else if(!wchan_isempty(NS->cv_wchan)){
                cv_broadcast(NS,intersectionLock);
             }
           }
           lock_release(intersectionLock);
    }
    else if((origin == north && destination == east) || (origin == south && destination == west)){
		      lock_acquire(intersectionLock);
          CarInIntersection--;
          if(CarInIntersection==0){
            if(!wchan_isempty(NS->cv_wchan)){
                cv_broadcast(NS,intersectionLock);
             }
             else if(!wchan_isempty(WE_L->cv_wchan)){
                cv_broadcast(WE_L,intersectionLock);
             }
             else if(!wchan_isempty(WE->cv_wchan)){
                cv_broadcast(WE,intersectionLock);
             }
		         else if(!wchan_isempty(NS_L->cv_wchan)){
                cv_broadcast(NS_L,intersectionLock);
             }
          }
          lock_release(intersectionLock);
    }
    else if((origin == west && destination == east) || (origin == east && destination == west) ||
         (origin == west && destination == south) || (origin == east && destination == north)){
           lock_acquire(intersectionLock);
           CarInIntersection--;
           if(CarInIntersection==0){
             if(!wchan_isempty(NS_L->cv_wchan)){
                cv_broadcast(NS_L,intersectionLock);
             }
             else if(!wchan_isempty(NS->cv_wchan)){
                cv_broadcast(NS,intersectionLock);
             }
             else if(!wchan_isempty(WE_L->cv_wchan)){
                cv_broadcast(WE_L,intersectionLock);
             }
		         else if(!wchan_isempty(WE->cv_wchan)){
                cv_broadcast(WE,intersectionLock);
             }
           }
           lock_release(intersectionLock);
    }
    else{
		   lock_acquire(intersectionLock);
       CarInIntersection--;
       if(CarInIntersection==0){
         if(!wchan_isempty(WE->cv_wchan)){
            cv_broadcast(WE,intersectionLock);
          }
          else if(!wchan_isempty(NS_L->cv_wchan)){
            cv_broadcast(NS_L,intersectionLock);
          }
          else if(!wchan_isempty(NS->cv_wchan)){
            cv_broadcast(NS,intersectionLock);
          }
		      else if(!wchan_isempty(WE_L->cv_wchan)){
             cv_broadcast(WE_L,intersectionLock);
          }
       }
       lock_release(intersectionLock);
    }
}
