#include <stdio.h>
#include "encrypt-module.h"

void reset_requested() {
	log_counts();
}

void reset_finished() {
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Incorrect arguments.\nCorrect Usage: `encrypt <input_file> <output_file> <log_file>`\n");
    return 1;
  }
	// init("in.txt", "out.txt", "log.txt"); 
  init(argv[0], argv[1], argv[2]);
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
