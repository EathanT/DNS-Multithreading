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
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN
#define INPUTFS "%1024s"
#define waittime rand() % 100

// Global variables
pthread_mutex_t qMutex;
pthread_mutex_t rMutex;
queue q;
int numFiles = 0;
FILE* inputfp[MAX_INPUT_FILES];
FILE* outputfp = NULL;
pthread_t requestThreads[MAX_INPUT_FILES];
pthread_t resolverThreads[MAX_RESOLVER_THREADS];

// Functions 
int queuePush(char* hostname);
char* queuePop();

int writeToOutput(char* domainName, char* ip);

void* request(void* args);
void* resolve();

#endif