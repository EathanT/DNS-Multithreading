#include "multi-lookup.h"

int liveProducers = 0;

int main(int argc, char* argv[]) {
    if (VERBOSE) printf("[Main] Program start, args=%d\n", argc);

    pthread_mutex_init(&qMutex, NULL);
    pthread_mutex_init(&rMutex, NULL);

    srand(time(NULL));
    if (VERBOSE) printf("[Main] RNG seeded\n");

    queue_init(&q, QUEUEMAXSIZE);
    if (VERBOSE) printf("[Main] Queue initialized (max %d)\n", QUEUEMAXSIZE);
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input1>... <output>\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc - 2 > MAX_INPUT_FILES) {
        fprintf(stderr, "Error: too many input files (max %d)\n", MAX_INPUT_FILES);
        return EXIT_FAILURE;
    }

    int numFiles = argc - 2;
    liveProducers = numFiles;

    for (int i = 0; i < numFiles; i++) {
        inputfp[i] = fopen(argv[1 + i], "r");
        if (!inputfp[i]) {
            fprintf(stderr, "Error opening %s\n", argv[1 + i]);
            return EXIT_FAILURE;
        }
        if (VERBOSE) printf("[Main] Opened input[%d]=%s\n", i, argv[1 + i]);
    }

    outputfp = fopen(argv[argc - 1], "w");
    if (!outputfp) {
        fprintf(stderr, "Error opening output %s\n", argv[argc - 1]);
        return EXIT_FAILURE;
    }
    if (VERBOSE) printf("[Main] Opened output=%s\n", argv[argc - 1]);

    // Start request threads
    for (int i = 0; i < numFiles; i++) {
        pthread_create(&requestThreads[i], NULL, request, &inputfp[i]);
        if (VERBOSE) printf("[Main] Started request thread %d\n", i);
    }

    // Start resolver threads
    for (int i = 0; i < MAX_RESOLVER_THREADS; i++) {
        pthread_create(&resolverThreads[i], NULL, resolve, NULL);
        if (VERBOSE) printf("[Main] Started resolver thread %d\n", i);
    }

    // Wait for producers
    for (int i = 0; i < numFiles; i++) {
        pthread_join(requestThreads[i], NULL);
        if (VERBOSE) printf("[Main] Request thread %d done\n", i);
    }

    // Wait for consumers
    for (int i = 0; i < MAX_RESOLVER_THREADS; i++) {
        pthread_join(resolverThreads[i], NULL);
        if (VERBOSE) printf("[Main] Resolver thread %d done\n", i);
    }

    pthread_mutex_destroy(&qMutex);
    pthread_mutex_destroy(&rMutex);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);
    
    if (VERBOSE) printf("[Main] Mutexes destroyed\n");

    queue_cleanup(&q);
    if (VERBOSE) printf("[Main] Queue cleaned\n");

    for (int i = 0; i < numFiles; i++) fclose(inputfp[i]);
    fclose(outputfp);
    if (VERBOSE) printf("[Main] Files closed, exiting\n");
    return EXIT_SUCCESS;
}

int queuePush(char* hostname) {
    pthread_mutex_lock(&qMutex);

    // wait for room
    while(queue_is_full(&q)){
        pthread_cond_wait(&not_full, &qMutex);
    }

    if (queue_push(&q, hostname) == QUEUE_FAILURE) {
        fprintf(stderr, "Queue push failed\n");
        pthread_mutex_unlock(&qMutex);
        return EXIT_FAILURE;
    }
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&qMutex);

    if (VERBOSE) printf("[Queue] Pushed '%s'\n", hostname ? hostname : "<NULL>");
    return EXIT_SUCCESS;
}

char* queuePop() {
    pthread_mutex_lock(&qMutex);
    while (queue_is_empty(&q) && liveProducers > 0) {
        pthread_cond_wait(&not_empty, &qMutex);
    }

    if(queue_is_empty(&q) && liveProducers == 0){
        pthread_mutex_unlock(&qMutex);
        return NULL;
    }

    char* h = (char*)queue_pop(&q);
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&qMutex);

    if (VERBOSE) printf("[Queue] Popped '%s'\n", h ? h : "<NULL>");
    return h;
}

int writeToOutput(char* domain, char* ip) {
    pthread_mutex_lock(&rMutex);
    fprintf(outputfp, "%s,%s\n", domain, ip);
    fflush(outputfp);
    pthread_mutex_unlock(&rMutex);

    if (VERBOSE) printf("[Output] %s,%s\n", domain, ip);
    return EXIT_SUCCESS;
}

void* request(void* arg) {
    FILE* fp = *(FILE**)arg;
    char host[MAX_NAME_LENGTH];
    while (fscanf(fp, INPUTFS, host) > 0) {
        queuePush(strdup(host));
    }

    // signal that one producer is done
    pthread_mutex_lock(&qMutex);
    liveProducers--;
    pthread_cond_broadcast(&not_empty);
    pthread_mutex_unlock(&qMutex);

    if (VERBOSE) printf("[Request] Thread %lu finished\n", pthread_self());
    return NULL;
}

void* resolve() {
    char ip[MAX_IP_LENGTH];
    if (VERBOSE) printf("[Resolve] Thread %lu starting\n", pthread_self());
   
    while (1) {   
        char* host = queuePop();
        if (!host) break;
        
        if (dnslookup(host, ip, sizeof(ip)) == EXIT_FAILURE) {
            fprintf(stderr, "dnslookup error: %s\n", host);
            ip[0] = '\0';
        }

        writeToOutput(host, ip);
        free(host);
    }
    if (VERBOSE) printf("[Resolve] Thread %lu exiting\n", pthread_self());
    return NULL;
}
