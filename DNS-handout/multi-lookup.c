#include "multi-lookup.h"

int main(int argc, char* argv[]){
    if (BENCHMARKING && currentBenchThreads == 0) {
        if (!benchmark(argc, argv)) {
            fprintf(stderr, "Unable to benchmark\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS; // done benchmarking
    }

    return run_threads(argc, argv, get_core_count());
}

int run_threads(int argc, char* argv[], int numResolverThreads){
    clock_t start_time, end_time;

    start_time = clock();

    pthread_mutex_init(&qMutex, NULL);
    pthread_mutex_init(&rMutex, NULL);

    srand(time(NULL));

    int maxResolverThreads = numResolverThreads;
    pthread_t requestThreads[MAX_INPUT_FILES];
    pthread_t resolverThreads[maxResolverThreads];


    if(VERBOSE) printf("[Main] Core Count: %d\n", get_core_count());

    if (VERBOSE) printf("[Main] Program start, args=%d\n", argc);



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

    outputfn = argv[argc - 1];
    outputfp = fopen(outputfn, "w+");
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
    for (int i = 0; i < maxResolverThreads; i++) {
        pthread_create(&resolverThreads[i], NULL, resolve, NULL);
        if (VERBOSE) printf("[Main] Started resolver thread %d\n", i);
    }

    // Wait for producers
    for (int i = 0; i < numFiles; i++) {
        pthread_join(requestThreads[i], NULL);
        if (VERBOSE) printf("[Main] Request thread %d done\n", i);
    }

    for (int i = 0; i < maxResolverThreads; i++) {
        queuePush(NULL);
    }

    // Wait for consumers
    for (int i = 0; i < maxResolverThreads; i++) {
        pthread_join(resolverThreads[i], NULL);
        if (VERBOSE) printf("[Main] Resolver thread %d done\n", i);
    }
    

    pthread_mutex_destroy(&qMutex);
    pthread_mutex_destroy(&rMutex);
    if (VERBOSE) printf("[Main] Mutexes destroyed\n");

    queue_cleanup(&q);
    if (VERBOSE) printf("[Main] Queue cleaned\n");

    for (int i = 0; i < numFiles; i++) fclose(inputfp[i]);
    fclose(outputfp);
    if (VERBOSE) printf("[Main] Files closed, exiting\n");

    end_time = clock();

    lastMainTime = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    return EXIT_SUCCESS;
}

int queuePush(char* hostname) {
    while (queue_is_full(&q)){
        usleep(WAITTIME);
    }

    pthread_mutex_lock(&qMutex);
    if (queue_push(&q, hostname) == QUEUE_FAILURE) {
        fprintf(stderr, "Queue push failed\n");
        pthread_mutex_unlock(&qMutex);
        return EXIT_FAILURE;
    }
    pthread_mutex_unlock(&qMutex);
    if (VERBOSE) printf("[Queue] Pushed '%s'\n", hostname ? hostname : "<NULL>");
    return EXIT_SUCCESS;
}

char* queuePop() {
    pthread_mutex_lock(&qMutex);
    while (queue_is_empty(&q)) {
        pthread_mutex_unlock(&qMutex);
        usleep(WAITTIME);
        pthread_mutex_lock(&qMutex);

    }
    char* h = (char*)queue_pop(&q);
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
        if(host[0] == '\0'){
            continue;
        }

        if(queuePush(strdup(host)) == QUEUE_SUCCESS){
            continue;
        }
    }

    
    while(1){
        if(query(host) == 1){
            break;
        }else if(VERBOSE){
            if(VERBOSE) printf("[Request] Couldnt Find %s in Output File\n", host);
        }

        usleep(QUERYTIME);
    }

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


int query(char host[MAX_NAME_LENGTH]) {
    int found = 0, lines = 0;
    FILE *file = fopen(outputfn, "r");
    if(!file) return 0;

    char line[2048];
while (fgets(line, sizeof line, file)) {
        lines++;
        if(VERBOSE) printf("[Query] searching for \"%s\" in \"%s\"\n", host, line);
        if (strstr(line, host)) {
            found = 1;
            break;
        }
    }
    fclose(file);

    if(VERBOSE) printf("[Query] saw %d lines, found %s? %s\n",
       lines, host, found ? "yes" : "no");
    
    return found;
}

int benchmark(int argc, char* argv[]) {
    FILE* f = fopen("benchmarks.txt", "w");
    if (!f) {
        fprintf(stderr, "Cannot open benchmarks.txt for writing\n");
        return 0;
    }

    // CSV Format
    fprintf(f, "Cores,Threads,Time(s)\n");

    // Loops through coreCounts array, and then also loops through
    // different amounts of threads for the core count.
    for (int ci = 0; ci < nSimulated; ci++) {
        int cores = simulatedCoreCounts[ci];
        int bestThreads = -1;
        double bestTime = -1.0;

        for (int t = 1; t <= cores * 3; t++) {
            currentBenchThreads = t;
            if (VERBOSE)
                printf("[Benchmark] Sim Cores=%d, Threads=%d\n", cores, t);

            if (run_threads(argc, argv, t) != EXIT_SUCCESS) {
                fprintf(f, "%d,%d,ERROR\n", cores, t);
            } else {
                fprintf(f, "%d,%d,%.6f\n", cores, t, lastMainTime);
                if (bestTime < 0 || lastMainTime < bestTime) {
                    bestTime = lastMainTime;
                    bestThreads = t;
                }
            }
        }

        fprintf(f,
            "\n"
            "### Simulated %d cores\n"
            "- Optimal threads: %d\n"
            "- Time: %.6f s\n\n",
            cores, bestThreads, bestTime);
    }

    fclose(f);
    return 1;
}
