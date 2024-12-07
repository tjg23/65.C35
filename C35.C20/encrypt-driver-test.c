#include <fcntl.h>
#include "encrypt-module.h"
#include "circular-buffer.h"
#include "reset-controller.h"

CircularBuffer *input_buffer;
CircularBuffer *output_buffer;

ResetController *rc;

int init_buffers() {
  int input_size, output_size;

  input_buffer = malloc(sizeof(CircularBuffer));
  output_buffer = malloc(sizeof(CircularBuffer));

  printf("Enter input buffer size: ");
  scanf("%d", &input_size);
  printf("Enter output buffer size: ");
  scanf("%d", &output_size);
  if (cb_init(input_buffer, input_size) != 0) {
    printf("Fatal: Failed to initialize input buffer\n");
    return 1;
  }
  if (cb_init(output_buffer, output_size) != 0) {
    printf("Fatal: Failed to initialize output buffer\n");
    return 1;
  }

  return 0;
}

void destroy_buffers() {
  cb_destroy(input_buffer);
  cb_destroy(output_buffer);
  free(input_buffer);
  free(output_buffer);
}

void *reader() {
  char c;
  while (1) {
    if (thread_block(rc, 0)) {
      continue;
    }

    c = read_input();
    cb_put(input_buffer, c);
    if (c == EOF) {
      return 0;
    }
  }
}

void *input_counter() {
  char c;
  while (1) {
    if (thread_block(rc, 1)) {
      continue;
    }

    c = cb_get(input_buffer, 0);
    if (c == EOF) {
      return 0;
    }
    count_input(c);
  }
}

void *encryptor() {
  char c, e;
  while (1) {
    if (thread_block(rc, 2)) {
      continue;
    }

    c = cb_get(input_buffer, 1);
    if (c == EOF) {
      cb_put(output_buffer, c);
      return 0;
    }
    e = encrypt(c);
    cb_put(output_buffer, e);
  }
}

void *output_counter() {
  char c;
  while (1) {
    if (thread_block(rc, 3)) {
      continue;
    }

    c = cb_get(output_buffer, 0);
    if (c == EOF) {
      return 0;
    }
    count_output(c);
  }
}

void *writer() {
  char c;
  while (1) {
    if (thread_block(rc, 4)) {
      continue;
    }

    c = cb_get(output_buffer, 1);
    if (c == EOF) {
      return 0;
    }
    write_output(c);
  }
}

void reset_requested() {
  pthread_mutex_lock(rc->reset_mutex);
  printf("Reset Requested.\n");

  int inputs = get_input_total_count();
  int outputs = get_output_total_count();

  printf("| Inputs: %d / Outputs: %d\n", inputs, outputs);
  if (inputs < outputs) {
    sem_post(rc->sem_thread_lock[1]);
    sem_post(rc->sem_thread_lock[2]);
  }
  if (inputs > outputs) {
    sem_post(rc->sem_thread_lock[2]);
    sem_post(rc->sem_thread_lock[3]);
    sem_post(rc->sem_thread_lock[4]);
  }
  rc->reset_in_progress = 1;

  if (inputs != outputs) {
    printf("Waiting for input and output to synchronize...\n");
    pthread_cond_wait(rc->reset_ready, rc->reset_mutex);
  }
  printf("Counts are synced. Logging counts.\n");

  printf("Performing reset.\n");
  pthread_mutex_unlock(rc->reset_mutex);
}

void reset_finished() {
  pthread_mutex_lock(rc->reset_mutex);
  printf("Reset finished.\n");

  rc_clear(rc);

  printf("Resuming blocked threads.\n\n");
  pthread_cond_broadcast(rc->reset_cond);

  pthread_mutex_unlock(rc->reset_mutex);
}

int main(int argc, char *argv[]) {

  if (argc != 3) {
    printf("Incorrect arguments.\nCorrect Usage: `encrypt-test <input_file> <output_file> `\n");
    return 1;
  }
	// init("in.txt", "out.txt", "log.txt"); 
  init(argv[1], argv[2], argv[3]);

  if (init_buffers()) {
    return 1;
  }

  rc = malloc(sizeof(ResetController));
  rc_init(rc);

  pthread_t read_thread, input_count_thread, encrypt_thread, output_count_thread, write_thread;
  pthread_create(&read_thread, NULL, &reader, NULL);
  pthread_create(&input_count_thread, NULL, &input_counter, NULL);
  pthread_create(&encrypt_thread, NULL, &encryptor, NULL);
  pthread_create(&output_count_thread, NULL, &output_counter, NULL);
  pthread_create(&write_thread, NULL, &writer, NULL);

  pthread_join(read_thread, NULL);
  pthread_join(input_count_thread, NULL);
  pthread_join(encrypt_thread, NULL);
  pthread_join(output_count_thread, NULL);
  pthread_join(write_thread, NULL);

	printf("End of file reached.\n"); 
  destroy_buffers();
}
