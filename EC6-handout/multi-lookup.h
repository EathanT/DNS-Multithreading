#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

// Includes
#include <cstdio>
#include <cstdlib>
#include "queue.h"
#include "util.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstring>

using namespace std;

// Definations
#define MAX_INPUT_FILES 10
#define INPUTFS "%1024s"
#define MAX_RESOLVER_THREADS 10
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

// Global Variables
FILE* inputfp[MAX_INPUT_FILES];
FILE* outputfp;
queue q;
int liveProducers = 0;
mutex qMutex;
condition_variable notempty, notfull;

void queuePush(char* hostname);
char* queuePop();

void request(int fileI);
void resolve();



#endif