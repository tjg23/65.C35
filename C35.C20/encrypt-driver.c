/**********************************************************
 * The main source file for the encrypt driver.           *
 * Contains the declarations for the input and output     *
 * buffers and the global ResetController. Defines the    *
 * functions to be called by the five threads and creates *
 * the threads to call them in `main()`. Implements the   *
 * functions `reset_requested()` and `reset_finished()`   *
 * as declared in `encrypt-module.h`, to be called from   *
 * `encrypt-module.c`.
 **********************************************************/
#include <fcntl.h>
#include "encrypt-module.h"
#include "circular-buffer.h"
#include "reset-controller.h"

/**
 * Declare global variables for the buffers and reset controller
 */
CircularBuffer *input_buffer;
CircularBuffer *output_buffer;

ResetController *rc;

/**
 * Initialize the buffers, prompting the user for their sizes.
 */
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

/**
 * Destroy the buffers and free their memory.
 */
void destroy_buffers() {
  cb_destroy(input_buffer);
  cb_destroy(output_buffer);
  free(input_buffer);
  free(output_buffer);
}

/**
 * Function to be run by the reader thread.
 */
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

/**
 * Function to be run by the input counter thread.
 */
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

/**
 * Function to be run by the encryptor thread.
 */
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

/**
 * Function to be run by the output counter thread.
 */
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

/**
 * Function to be run by the writer thread.
 */
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

/**
 * Called when the encrypt-module requests a reset, so the
 * input and output counts can be synchronized before the
 * reset is performed.
 * Acquires a lock on the global ResetController to block
 * all driver threads, then compares the total input and
 * output counts and resumes the side that is behind.
 * Waits for one of those threads to signal `reset_ready`
 * when the counts are equal before returning and allowing
 * the reset to complete.
 */
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

	log_counts();

  printf("Performing reset.\n");
  pthread_mutex_unlock(rc->reset_mutex);
}

/**
 * Called when the encrypt-module completes a reset so the
 * driver threads can be resumed.
 */
void reset_finished() {
  pthread_mutex_lock(rc->reset_mutex);
  printf("Reset finished.\n");

  rc_clear(rc);

  printf("Resuming blocked threads.\n\n");
  pthread_cond_broadcast(rc->reset_cond);

  pthread_mutex_unlock(rc->reset_mutex);
}

/** Main function
 * Entry point of the program - reads the input file name,
 * output file name, and log file name from the arguments
 * and initializes the encrypt-module, then initializes the
 * input and output buffers and the reset controller.
 * Finally creates the five driver threads and waits for them
 * to complete and logs the final input and output counts.
 */
int main(int argc, char *argv[]) {

  if (argc != 4) {
    printf("Incorrect arguments.\nCorrect Usage: `encrypt <input_file> <output_file> <log_file>`\n");
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
	log_counts();
}
