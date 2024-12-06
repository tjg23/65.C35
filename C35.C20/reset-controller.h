#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

typedef struct {
  int reset_in_progress;

  pthread_mutex_t *reset_mutex;
  pthread_cond_t *reset_cond;

  sem_t *sem_thread_lock[5];
} ResetController;

void rc_init(ResetController *rc) {
  rc->reset_in_progress = 0;

  pthread_mutex_init(rc->reset_mutex, NULL);
  pthread_cond_init(rc->reset_cond, NULL);

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

int thread_block(ResetController *rc, int thread) {
  pthread_mutex_lock(rc->reset_mutex);
  if (!rc->reset_in_progress) {
    pthread_mutex_unlock(rc->reset_mutex);
    return 0;
  }

  int s = sem_trywait(rc->sem_thread_lock[thread]);
  if (!s) {
    pthread_mutex_unlock(rc->reset_mutex);
    return 0;
  }

  if (rc->reset_in_progress) {
    pthread_cond_wait(rc->reset_cond, rc->reset_mutex);
  }

  pthread_mutex_unlock(rc->reset_mutex);
  return 1;
}
