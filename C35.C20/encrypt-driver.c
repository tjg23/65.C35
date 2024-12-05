#include <stdio.h>
#include "encrypt-module.h"
#include "circular-buffer.h"

CircularBuffer *input_buffer;
CircularBuffer *output_buffer;

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

void reset_requested() {
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

	char c;
	while ((c = read_input()) != EOF) { 
		count_input(c); 
		c = encrypt(c); 
		count_output(c); 
		write_output(c); 
	} 
	printf("End of file reached.\n"); 
	log_counts();
}
