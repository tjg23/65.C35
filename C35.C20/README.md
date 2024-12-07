# CS 352 Project 2
### Author: Tyler Gorton | tjgorton@iastate.edu

This project implements a multi-threaded text file encryptor, using semaphores,
mutexes, and condition variables for synchronization.

## Building & Running
This project inludes a makefile with a few targets for building and running
the program. The default target `build` will compile the source code into
an executable called `encrypt`. The target `run` will perform `build` and
then run the resulting executable with default arguments ('in.txt out.txt log.txt').
The target `start` will run the executable with default arguments, assuming
it has already been built.

## Project Layout
The bulk of the project code is contained in `encrypt-driver.c`. Data structures
and relevant helper methods have been extracted into two header files:
`circular-buffer.h` and `reset-controller.h`.

### Circular Buffer
The data structure utilized for the input and output buffers is a circular buffer,
defined by the `CircularBuffer` struct in `circular-buffer.h`. 

This structure includes a dynamically allocated `char` array, an `int` variable 
for the buffer's total size, a `tail` index for the producer to write to, and 
two `head` indexes for the two consumers to read from. It also provides a mutex
for synchronization and four semaphores - two `read` and two `full` - to 
coordinate between the producer and consumers.

This file also provides the helper functions to initialize a buffer (`cb_init`),
add a character (`cb_put`), get a character (`cb_get`), and destroy the buffer
(`cb_destroy`).

### Reset Controller
Handling the encryption module reset is done with the help of the `ResetController`
struct defined in `reset-controller.h`.

This structure utilizes a mutex for thread-safe access and provides several
mechanisms for coordinating a reset. The flag `int reset_in_progress` tracks
if a reset request is being processed, and the condition variables `reset_ready`
and `reset_cond` are used to signal when the reset is ready to be performed and
when it is finished, respectively.
The struct also contains five semaphores, declared in the pointer array
`sem_thread_lock`, which are used to more precisely control the operation of
the driver's threads for the purpose of coordinating for a reset. They allow
the driver to process either the input or the output, depending on which one
is behind, so that the counts can be synchronized and the reset can occur.

This file also provides a helper function to initialize the `ResetController`
object (`rc_init`), a helper to check if a thread is allowed to continue
(`thread_block`), and a function to clear the state of the reset controller
after a reset is completed.

### Main
The main file of the project is `encrypt-driver.c`. This file declares global
variables for the input and output buffers and the reset controller, defines
the functions to be performed by the five driver threads, and implements the
`reset_requested()` and `reset_finished()` functions to be called by the
encrypt module when it performs a reset.

The `main` function gets the input, output, and log file names from the 
program arguments and initializes the encrypt module. Then it initializes
the buffers and the reset controller. Finally, it spawns the five processing
threads and waits for them all to complete.

### Encrypt Module
The I/O and encryption functions are declared in `encrypt-module.h` and implemented
in `encrypt-module.c`. These files are unchanged.
