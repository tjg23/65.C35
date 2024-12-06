#include <stdio.h>
#include "encrypt-module.h"
#include "circular-buffer.h"

CircularBuffer *input_buffer;
CircularBuffer *output_buffer;

sem_t *sem_input_lock;
sem_t *sem_output_lock;

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
  while ((c = read_input()) != EOF) {
    cb_put(input_buffer, c);
  }
  cb_put(input_buffer, c);
  return 0;
  // while (true) {
    // sem_wait(sem_input_lock);
    // c = read_input();
    // cb_put(input_buffer, c);
    // sem_post(sem_input_lock);
    // if (c == EOF) {
      // return;
    // }
  // }
}

void *input_counter() {
  char c;
  while ((c = cb_get(input_buffer, 0)) != EOF) {
    count_input(c);
  }
  return 0;
  // while (true) {
    // sem_wait(sem_input_lock);
    // c = cb_get(input_buffer, 0);
    // if (c != EOF) {
      // count_input(c);
    // }
    // sem_post(sem_input_lock);
    // if (c == EOF) {
      // return;
    // }
  // }
}

void *encryptor() {
  char c, e;
  while ((c = cb_get(input_buffer, 1)) != EOF) {
    e = encrypt(c);
    cb_put(output_buffer, e);
  }
  cb_put(output_buffer, c);
  return 0;
}

void *output_counter() {
  char c;
  while ((c = cb_get(output_buffer, 0)) != EOF) {
    count_output(c);
  }
  return 0;
  // while (true) {
    // sem_wait(sem_output_lock);
    // c = cb_get(output_buffer, 0);
    // if (c != EOF) {
      // count_output(c);
    // }
    // sem_post(sem_output_lock);
    // if (c == EOF) {
      // return;
    // }
  // }
}

void *writer() {
  char c;
  while ((c = cb_get(output_buffer, 1)) != EOF) {
    write_output(c);
  }
  write_output(c);
  return 0;
  // while (true) {
    // sem_wait(sem_output_lock);
    // c = cb_get(output_buffer, 1);
    // write_output(c);
    // sem_post(sem_output_lock);
    // if (c == EOF) {
      // return;
    // }
  // }
}

void reset_requested() {
  // sem_wait(sem_input_lock);
  // sem_wait(sem_input_lock);
  // sem_wait(sem_output_lock);
  // sem_wait(sem_output_lock);

  int inputs = get_input_total_count();
  int outputs = get_output_total_count();

  // while ()

	log_counts();
}

void reset_finished() {
}

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

  sem_input_lock = sem_open("/sem_input_lock", O_CREAT, 0644, 2);
  sem_unlink("/sem_input_lock");
  sem_output_lock = sem_open("/sem_output_lock", O_CREAT, 0644, 2);
  sem_unlink("/sem_output_lock");

	// char c;
	// while ((c = read_input()) != EOF) { 
  //	count_input(c); 
	//	c = encrypt(c); 
	//	count_output(c); 
	//	write_output(c); 
	//} 
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
