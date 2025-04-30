#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// Shared resource
int counter = 0;

// Mutex to protect the shared resource
pthread_mutex_t mutex;

// Function executed by threads
void* increment_counter(void* arg) {
    for (int i = 0; i < 5; i++) {
        // Lock the mutex before accessing the shared resource
       // pthread_mutex_lock(&mutex);

        // Critical section
        int temp = counter;
        printf("Thread %ld: Counter = %d\n", (long)arg, temp);
        counter = temp + 1;

        // Unlock the mutex after accessing the shared resource
       // pthread_mutex_unlock(&mutex);

        // Simulate some work
        usleep(100000);
    }
    return NULL;
}

int main() {
    // Initialize the mutex
    pthread_mutex_init(&mutex, NULL);

    // Create threads
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, increment_counter, (void*)1);
    pthread_create(&thread2, NULL, increment_counter, (void*)2);

    // Wait for threads to finish
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // Destroy the mutex
    pthread_mutex_destroy(&mutex);

    printf("Final Counter Value: %d\n", counter);
    return 0;
}