/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>

#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
// bool semaphore_priority_more(const struct list_elem *a,
//       const struct list_elem *b,
//       void *aux UNUSED)
// {
// const struct semaphore_elem *sema_a = list_entry(a, struct semaphore_elem, elem);
// const struct semaphore_elem *sema_b = list_entry(b, struct semaphore_elem, elem);

// struct thread *t_a = list_entry(list_front(&sema_a->semaphore.waiters),
//           struct thread, elem);
// struct thread *t_b = list_entry(list_front(&sema_b->semaphore.waiters),
//           struct thread, elem);

// return t_a->priority > t_b->priority;
// }
/* One semaphore in a list. */
struct semaphore_elem 
  {
    struct list_elem elem;              /* List element. */
    struct semaphore semaphore;         /* This semaphore. */
    int sema_elem_prioirty;
  };
bool semaElement_priority_more(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED) {
  return list_entry(a, struct semaphore_elem, elem)->sema_elem_prioirty > list_entry(b,  struct semaphore_elem, elem)->sema_elem_prioirty;
}

void
sema_init (struct semaphore *sema, unsigned value) 
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
      list_insert_ordered (&sema->waiters, &thread_current ()->elem, thread_priority_more, NULL);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}


/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0) 
    {
      sema->value--;
      success = true; 
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. 
*/
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;
  sema->value++;
  ASSERT (sema != NULL);
  
  old_level = intr_disable ();
  if (!list_empty (&sema->waiters)) {
    list_sort(&sema->waiters,thread_priority_more,NULL);
    thread_unblock (list_entry (list_pop_front (&sema->waiters),
                                struct thread, elem));   
  }      
  intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) 
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{ 
  
  ASSERT (lock != NULL);
  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));
  
  if(lock->holder!= NULL&&!thread_mlfqs){
    struct lock_elem *le = malloc(sizeof( struct lock_elem));
    le->myLock= lock;
    
    int maxWaiter = -1  ;
  
    int  max_pri= thread_current()->priority;
     if (!list_empty(&lock->semaphore.waiters)) {
       maxWaiter = list_entry(list_front(&lock->semaphore.waiters), struct thread, elem)->effective_priority;
       if(maxWaiter>max_pri){
        max_pri=maxWaiter;
      }
    }
    le->priority= max_pri;
    list_insert_ordered(&lock->holder->eff_priority_list, &le->elem , priority_elem_more, NULL);
  
    donation(thread_current(),lock);
    
  }

  
  sema_down (&lock->semaphore);
  
  
  lock->holder = thread_current();
}
void 
donation (struct thread *donator , struct lock * lock){
  int first = list_entry(list_front(& lock->holder->eff_priority_list),struct lock_elem, elem)->priority;
  int highest_donation = donator->effective_priority>first ?donator->effective_priority :first;
  donator->waitting = lock ;
  if (donator->effective_priority >= lock->holder->effective_priority){
  
    lock->holder->effective_priority = highest_donation > lock->holder->effective_priority ? highest_donation :lock->holder->effective_priority;
    list_entry(list_front(& lock->holder->eff_priority_list),struct lock_elem, elem)->priority = lock->holder->effective_priority;
    if (lock->holder->waitting != NULL){
      donation(lock->holder,lock->holder->waitting);
    }
  }
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = thread_current ();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  if(!thread_mlfqs){
    
    struct thread * t = thread_current();
    struct list_elem *e = list_begin(&t->eff_priority_list);
    
    
    while (e != list_end(&t->eff_priority_list)) {
      struct lock_elem *d = list_entry(e, struct lock_elem, elem);
      
      if (d->myLock == lock) {
        
        list_remove(e);	

      }		
      e = list_next(e);
    }

    
    int new_priority =lock->holder->priority;
    
    if(!list_empty(&lock->holder->eff_priority_list)) {
      new_priority = list_entry(list_front(&lock->holder->eff_priority_list), struct lock_elem, elem)->priority;
      
      
      new_priority = new_priority >lock->holder->priority ?new_priority:lock->holder->priority;
    } 
    thread_current()->effective_priority=new_priority;
    
  }

  lock->holder = NULL;
  sema_up (&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}



/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
   
  
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  struct semaphore_elem waiter;
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  sema_init (&waiter.semaphore, 0);
  waiter.sema_elem_prioirty = thread_current()->priority;
  list_insert_ordered(&cond->waiters, &waiter.elem,semaElement_priority_more, NULL);
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);

}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
  
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  
  if (!list_empty (&cond->waiters)) {
    sema_up (&list_entry (list_pop_front(&cond->waiters),
                          struct semaphore_elem, elem)->semaphore);
    
  }
}
   
/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}