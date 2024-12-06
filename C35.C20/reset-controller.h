#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

typedef struct {
  int reset_in_progress;

  pthread_mutex_t reset_mutex;
  pthread_cond_t *reset_cond;

  sem_t *sem_thread_lock[5];
} ResetController;

int rc_init(ResetController *rc) {
  printf("| reset_in_progress = 0\n");
  rc->reset_in_progress = 0;

  printf("| init reset_mutex\n");
  if (pthread_mutex_init(&rc->reset_mutex, NULL) != 0) {
    printf("! -- Mutex initialization failed --");
    return -1;
  }
  printf("| init reset_cond\n");
  rc->reset_cond = malloc(sizeof(pthread_cond_t));
  pthread_cond_init(rc->reset_cond, NULL);

  // rc->sem_thread_lock[0] = sem_open("/sem_read_lock", O_CREAT, 0644, 0);
  // if (rc->sem_thread_lock[0] == SEM_FAILED) {
    // printf("Failed to open semaphore `/sem_read_lock`\n");
    // return -1;
  // }
  // sem_unlink("/sem_read_lock");
  // rc->sem_thread_lock[1] = sem_open("/sem_icount_lock", O_CREAT, 0644, 0);
  // if (rc->sem_thread_lock[1] == SEM_FAILED) {
    // printf("Failed to open semaphore `/sem_icount_lock`\n");
    // return -1;
  // }
  // sem_unlink("/sem_icount_lock");
  // rc->sem_thread_lock[2] = sem_open("/sem_encrypt_lock", O_CREAT, 0644, 0);
  // if (rc->sem_thread_lock[2] == SEM_FAILED) {
    // printf("Failed to open semaphore `/sem_encrypt_lock`\n");
    // return -1;
  // }
  // sem_unlink("/sem_encrypt_lock");
  // rc->sem_thread_lock[3] = sem_open("/sem_ocount_lock", O_CREAT, 0644, 0);
  // if (rc->sem_thread_lock[3] == SEM_FAILED) {
    // printf("Failed to open semaphore `/sem_ocount_lock`\n");
    // return -1;
  // }
  // sem_unlink("/sem_ocount_lock");
  // rc->sem_thread_lock[4] = sem_open("/sem_write_lock", O_CREAT, 0644, 0);
  // if (rc->sem_thread_lock[4] == SEM_FAILED) {
    // printf("Failed to open semaphore `/sem_write_lock`\n");
    // return -1;
  // }
  // sem_unlink("/sem_write_lock");
  printf("| init sem_thread_lock[0]\n");
  sem_init(rc->sem_thread_lock[0], 0, 0);
  printf("| init sem_thread_lock[1]\n");
  sem_init(rc->sem_thread_lock[1], 0, 0);
  printf("| init sem_thread_lock[2]\n");
  sem_init(rc->sem_thread_lock[2], 0, 0);
  printf("| init sem_thread_lock[3]\n");
  sem_init(rc->sem_thread_lock[3], 0, 0);
  printf("| init sem_thread_lock[4]\n");
  sem_init(rc->sem_thread_lock[4], 0, 0);
  
  return 0;
}

int thread_block(ResetController *rc, int thread) {
  pthread_mutex_lock(&rc->reset_mutex);
  if (!rc->reset_in_progress) {
    pthread_mutex_unlock(&rc->reset_mutex);
    return 0;
  }

  int s = sem_trywait(rc->sem_thread_lock[thread]);
  if (!s) {
    pthread_mutex_unlock(&rc->reset_mutex);
    return 0;
  }

  if (rc->reset_in_progress) {
    pthread_cond_wait(rc->reset_cond, &rc->reset_mutex);
  }

  pthread_mutex_unlock(&rc->reset_mutex);
  return 1;
}
