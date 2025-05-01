#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

// File Includes
#include "queue.h"
#include "util.h"

// Define constants
#define MAX_INPUT_FILES 10
#define MAX_NAME_LENGTH 1025
#define MIN_RESOLVER_THREADS 2
#define MAX_IP_LENGTH INET6_ADDRSTRLEN
#define INPUTFS "%1024s"
#define WAITTIME rand() % 100
#define QUERYTIME 250

// Global variables
pthread_mutex_t qMutex;
pthread_mutex_t rMutex;
queue q;
int numFiles = 0;
FILE* inputfp[MAX_INPUT_FILES];
FILE* outputfp = NULL;
char* outputfn = NULL;
int liveProducers = 0;

// All for bench marking
int BENCHMARKING = 1; 
int currentBenchThreads = 0;
double lastMainTime = 0;
static const int simulatedCoreCounts[] = { 1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20 };
static const int nSimulated = sizeof(simulatedCoreCounts) / sizeof(simulatedCoreCounts[0]);


// Runs main program, multithreaded DNS input to outpu 
int run_threads(int argc, char* argv[], int numResolverThreads);

// Pop and push to queue with mutex
int queuePush(char* hostname);
char* queuePop();

int writeToOutput(char* domainName, char* ip);

// Request and Resolve thread funcitons
void* request(void* args);
void* resolve();

// Go though output file and check if host is in the file
int query(char host[MAX_NAME_LENGTH]);

// Benchmarking core counts and thread amounts
int benchmark(int argc, char* argv[]);
#endif