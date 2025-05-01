#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include "queue.h"
#include "util.h"

// Define constants
#define MAX_INPUT_FILES 10
#define MAX_NAME_LENGTH 1025
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2
#define MAX_IP_LENGTH INET6_ADDRSTRLEN
#define INPUTFS "%1024s"
#define waittime rand() % 100
#define QUERYTIME 250

// Global variables
pthread_mutex_t qMutex;
pthread_mutex_t rMutex;
queue q;
int numFiles = 0;
int liveProducers = 0;
FILE* inputfp[MAX_INPUT_FILES];
FILE* outputfp = NULL;
char* outputfn = NULL;

// All for bench marking
int BENCHMARKING = 1; 
int currentBenchThreads = 0;
double lastMainTime = 0;

// Runs main program, multithreaded DNS input to outpu 
int run_threads(int argc, char* argv[], int numResolverThreads);

// Functions 
int queuePush(char* hostname);
char* queuePop();

int writeToOutput(char* domainName, char* ip);

void* request(void* args);
void* resolve();

// Go though output file and check if host is in the file
int query(char host[MAX_NAME_LENGTH]);

// Benchmarking core counts and thread amounts
int benchmark(int argc, char* argv[]);

#endif