/**********************************************************
 * This header defines the circular buffer data structure *
 * used for the input and output buffers. It contains a   *
 * dynamically allocated character array, a size value,   *
 * head and tail indexes, a mutex for synchronization,    *
 * and semaphores to coordinate producers and consumers.  *
 **********************************************************/ 
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

/** Circular Buffer Structure
 * The CircularBuffer struct contains a pointer to a dynamically
 * allocated character array, an int value for the buffer size,
 * and head and tail indexes. It also includes a mutex for 
 * thread-safe access and semaphores to coordinate between
 * producers and consumers. Because the buffers will each
 * have one producer and two consumers, the `head` index and
 * both semaphore types are duplicated as arrays of length 2.
 */ 
typedef struct {
    char *buffer;      // Actual buffer to store characters
    int size;          // Total size of the buffer
    // int count;         // Current number of elements in the buffer
    int head[2];       // Index to read from, one for each consumer
    int tail;          // Index to write to
    
    // Synchronization primitives
    pthread_mutex_t *mutex;       // Mutex for thread-safe access
    sem_t *read[2];               // Semaphore to track empty slots for each consumer
    sem_t *full[2];               // Semaphore to track filled slots for each consumer
} CircularBuffer;

/**
 * Initialize the CircularBuffer at `cb` with size `buffer_size`
 */
int cb_init(CircularBuffer *cb, int buffer_size) {
    // Allocate buffer
    cb->buffer = (char*) malloc(buffer_size * sizeof(char));
    if (cb->buffer == NULL) {
        printf("Buffer memory allocation failed\n");
        return -1;  // Memory allocation failed
    }
    
    // Initialize buffer properties
    cb->size = buffer_size;
    // cb->count = 0;
    cb->head[0] = 0;
    cb->head[1] = 0;
    cb->tail = 0;
    
    cb->mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    // Initialize synchronization primitives
    if (pthread_mutex_init(cb->mutex, NULL) != 0) {
        free(cb->buffer);
        free(cb->mutex);
        printf("Buffer mutex initialization failed\n");
        return -1;
    }

    cb->read[0] = sem_open("/sem_cb_read_1", O_CREAT, 0644, buffer_size);
    sem_unlink("/sem_cb_read_1");
 
    cb->read[1] = sem_open("/sem_cb_read_2", O_CREAT, 0644, buffer_size);
    sem_unlink("/sem_cb_read_2");

    cb->full[0] = sem_open("/sem_cb_post_1", O_CREAT, 0644, 0);
    sem_unlink("/sem_cb_post_1");

    cb->full[1] = sem_open("/sem_cb_post_2", O_CREAT, 0644, 0);
    sem_unlink("/sem_cb_post_2");
    
    return 0;
}

/**
 * Add an item to `cb`. Waits for both `read` semaphores 
 * to ensure that both consumers have processed at least
 * one cell in the buffer and posts to both `full` semaphores 
 * to signal to both consumers that an item was added.
 */
int cb_put(CircularBuffer *cb, char item) {
    // Wait for an empty slot
    sem_wait(cb->read[0]);
    sem_wait(cb->read[1]);
    
    // Acquire mutex to modify buffer
    pthread_mutex_lock(cb->mutex);
    
    // Add item to buffer
    cb->buffer[cb->tail] = item;
    cb->tail = (cb->tail + 1) % cb->size;
    // cb->count++;
    
    // Release mutex
    pthread_mutex_unlock(cb->mutex);
    
    // Signal that a slot is now full
    sem_post(cb->full[0]);
    sem_post(cb->full[1]);
    
    return 0;
}

/**
 * Remove and return an item from `cb`. `cid` represents
 * the calling consumer thread and can be either `0` or `1`.
 * This value is used as an index on the `head`, `full`, 
 * and `read` arrays so the two consumers are consistently
 * tracked independent of each other.
 */
char cb_get(CircularBuffer *cb, int cid) {
    // Wait for a filled slot
    sem_wait(cb->full[cid]);
    
    // Acquire mutex to modify buffer
    pthread_mutex_lock(cb->mutex);
    
    // Remove item from buffer
    char item = cb->buffer[cb->head[cid]];
    cb->head[cid] = (cb->head[cid] + 1) % cb->size;
    // cb->count--;
    
    // Release mutex
    pthread_mutex_unlock(cb->mutex);
    
    // Signal that an empty slot is now available
    sem_post(cb->read[cid]);
    
    return item;
}

// Check if buffer is empty (not thread-safe, use carefully)
// int cb_is_empty(CircularBuffer *cb) {
//     return cb->count == 0;
// }

/**
 * Cleanup function to close the semaphores, destroy the 
 * mutex, and free allocated memory for `cb`.
 */
void cb_destroy(CircularBuffer *cb) {
    // Destroy synchronization primitives
    sem_close(cb->read[0]);
    sem_close(cb->read[1]);
    sem_close(cb->full[0]);
    sem_close(cb->full[1]);
    pthread_mutex_destroy(cb->mutex);
    
    free(cb->mutex);
    free(cb->buffer);
}
