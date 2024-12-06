#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// Circular Buffer Structure
typedef struct {
    char *buffer;      // Actual buffer to store characters
    int size;          // Total size of the buffer
    // int count;         // Current number of elements in the buffer
    int head[2];       // Index to read from
    int tail;          // Index to write to
    
    // Synchronization primitives
    pthread_mutex_t mutex;       // Mutex for thread-safe access
    sem_t read[2];               // Semaphore to track empty slots
    sem_t full[2];               // Semaphore to track filled slots
} CircularBuffer;

// Initialize the circular buffer
int cb_init(CircularBuffer *cb, int buffer_size) {
    // Allocate buffer
    cb->buffer = malloc(buffer_size * sizeof(char));
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
    
    // Initialize synchronization primitives
    if (pthread_mutex_init(&cb->mutex, NULL) != 0) {
        free(cb->buffer);
        printf("Buffer mutex initialization failed\n");
        return -1;
    }
    
    if (sem_init(&cb->read[0], 0, buffer_size) != 0) {
        pthread_mutex_destroy(&cb->mutex);
        free(cb->buffer);
        printf("Buffer semaphore (read[0]) initialization failed\n");
        return -1;
    }

    if (sem_init(&cb->read[1], 0, buffer_size) != 0) {
        sem_destroy(&cb->read[0]);
        pthread_mutex_destroy(&cb->mutex);
        free(cb->buffer);
        printf("Buffer semaphore (read[1]) initialization failed\n");
        return -1;
    }

    
    if (sem_init(&cb->full[0], 0, 0) != 0) {
        sem_destroy(&cb->read[0]);
        sem_destroy(&cb->read[1]);
        pthread_mutex_destroy(&cb->mutex);
        free(cb->buffer);
        printf("Buffer semaphore (full[0]) initialization failed\n");
        return -1;
    }

    if (sem_init(&cb->full[1], 0, 0) != 0) {
        sem_destroy(&cb->read[0]);
        sem_destroy(&cb->read[1]);
        sem_destroy(&cb->full[0]);
        pthread_mutex_destroy(&cb->mutex);
        free(cb->buffer);
        printf("Buffer semaphore (full[1]) initialization failed\n");
        return -1;
    }
    
    return 0;
}

// Add an item to the circular buffer
int cb_put(CircularBuffer *cb, char item) {
    // Wait for an empty slot
    sem_wait(&cb->read[0]);
    sem_wait(&cb->read[1]);
    
    // Acquire mutex to modify buffer
    pthread_mutex_lock(&cb->mutex);
    
    // Add item to buffer
    cb->buffer[cb->tail] = item;
    cb->tail = (cb->tail + 1) % cb->size;
    // cb->count++;
    
    // Release mutex
    pthread_mutex_unlock(&cb->mutex);
    
    // Signal that a slot is now full
    sem_post(&cb->full[0]);
    sem_post(&cb->full[1]);
    
    return 0;
}

// Remove and return an item from the circular buffer
char cb_get(CircularBuffer *cb, int cid) {
    // Wait for a filled slot
    sem_wait(&cb->full[cid]);
    
    // Acquire mutex to modify buffer
    pthread_mutex_lock(&cb->mutex);
    
    // Remove item from buffer
    char item = cb->buffer[cb->head[cid]];
    cb->head[cid] = (cb->head[cid] + 1) % cb->size;
    // cb->count--;
    
    // Release mutex
    pthread_mutex_unlock(&cb->mutex);
    
    // Signal that an empty slot is now available
    sem_post(&cb->read[cid]);
    
    return item;
}

// Check if buffer is empty (not thread-safe, use carefully)
// int cb_is_empty(CircularBuffer *cb) {
//     return cb->count == 0;
// }

// Cleanup function
void cb_destroy(CircularBuffer *cb) {
    // Destroy synchronization primitives
    sem_destroy(&cb->read[0]);
    sem_destroy(&cb->read[1]);
    sem_destroy(&cb->full[0]);
    sem_destroy(&cb->full[1]);
    pthread_mutex_destroy(&cb->mutex);
    
    // Free buffer memory
    free(cb->buffer);
}
