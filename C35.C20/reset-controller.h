/**********************************************************
 * This file provides a struct and a helper function used *
 * for thread synchronization and module reset handling.  *
 * It uses various synchronization mechanisms including a *
 * mutex, two condition variables, and five semaphores to *
 * coordinate between the threads.                        *
 **********************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include "encrypt-module.h"

/**
 * ResetController contains a mutex for thread-safe access and
 * a flag for a reset in progress. It also provides two  
 * condition variables to coordinate when the reset is   
 * ready and when it's completed. Finally, it includes 5 
 * semaphores to allow controlled processing of specific 
 * threads so they can be synced before a reset.         
 */
typedef struct {
  int reset_in_progress;

  pthread_mutex_t *reset_mutex;
  pthread_cond_t *reset_cond;
  pthread_cond_t *reset_ready;

  sem_t *sem_thread_lock[5];
} ResetController;

/**
 * Initializes the ResetController at the given pointer.
 */
void rc_init(ResetController *rc) {
  rc->reset_in_progress = 0;

  rc->reset_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(rc->reset_mutex, NULL);
  rc->reset_cond = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
  pthread_cond_init(rc->reset_cond, NULL);
  rc->reset_ready = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
  pthread_cond_init(rc->reset_ready, NULL);

  rc->sem_thread_lock[0] = sem_open("/sem_read_lock", O_CREAT, 0644, 0);
  sem_unlink("/sem_read_lock");
  rc->sem_thread_lock[1] = sem_open("/sem_icount_lock", O_CREAT, 0644, 0);
  sem_unlink("/sem_icount_lock");
  rc->sem_thread_lock[2] = sem_open("/sem_encrypt_lock", O_CREAT, 0644, 0);
  sem_unlink("/sem_encrypt_lock");
  rc->sem_thread_lock[3] = sem_open("/sem_ocount_lock", O_CREAT, 0644, 0);
  sem_unlink("/sem_ocount_lock");
  rc->sem_thread_lock[4] = sem_open("/sem_write_lock", O_CREAT, 0644, 0);
  sem_unlink("/sem_write_lock");
}

/**
 * Determines if the thread represented by `int thread`
 * is allowed to perform its operation. Returns 0 if
 * the thread is able to proceed or 1 if it is blocked.
 */ 
int thread_block(ResetController *rc, int thread) {
  /* Lock `rc->reset_mutex` for safe concurrency */
  pthread_mutex_lock(rc->reset_mutex);
  /* If a reset is not in progress, allow the thread to continue */
  if (!rc->reset_in_progress) {
    pthread_mutex_unlock(rc->reset_mutex);
    return 0;
  }

  int s = sem_trywait(rc->sem_thread_lock[thread]);
  if (!s) {
    // printf("| Thread %d: ", thread);
    int i = get_input_total_count();
    int o = get_output_total_count();
    // printf("Inputs = %d / Outputs = %d\n", i, o);
    if (i == o) {
      pthread_cond_signal(rc->reset_ready);
      pthread_cond_wait(rc->reset_cond, rc->reset_mutex);
      pthread_mutex_unlock(rc->reset_mutex);
      return 1;
    }
    if (i < o) {
      sem_post(rc->sem_thread_lock[1]);
      sem_post(rc->sem_thread_lock[2]);
    }
    if (i > o) {
      sem_post(rc->sem_thread_lock[2]);
      sem_post(rc->sem_thread_lock[3]);
      sem_post(rc->sem_thread_lock[4]);
    }
    pthread_mutex_unlock(rc->reset_mutex);
    return 0;
  }

  if (rc->reset_in_progress) {
    pthread_cond_wait(rc->reset_cond, rc->reset_mutex);
  }

  pthread_mutex_unlock(rc->reset_mutex);
  return 1;
}

/**
 * Clear the state of the ResetController by setting
 * the `reset_in_progress` flag to 0 and consuming 
 * all of the semaphores.
 * Does not acquire a lock on `reset_mutex`, so should
 * be called from a scope that already owns the lock.
 */ 
int rc_clear(ResetController *rc) {
//   pthread_mutex_lock(rc->reset_mutex);

  rc->reset_in_progress = 0;

  while (!sem_trywait(rc->sem_thread_lock[0])) {}
  while (!sem_trywait(rc->sem_thread_lock[1])) {}
  while (!sem_trywait(rc->sem_thread_lock[2])) {}
  while (!sem_trywait(rc->sem_thread_lock[3])) {}
  while (!sem_trywait(rc->sem_thread_lock[4])) {}

  return 0;
}
