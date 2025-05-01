#include "multi-lookup.h"


int main(int argc, char* argv[]){
    if(argc < 3){
        fprintf(stderr, "Usage: %s <input1>... <output>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int numFiles = argc - 2;
    if (numFiles > MAX_INPUT_FILES) {
        fprintf(stderr,"Error: too many input files (max %d)\n",MAX_INPUT_FILES);
        return EXIT_FAILURE;
    }

    // open all inputs & output
    liveProducers = numFiles;
    queue_init(&q, QUEUEMAXSIZE);
    for (int i = 0; i < numFiles; i++) {
        inputfp[i] = fopen(argv[1+i], "r");
        if (!inputfp[i]) {
            fprintf(stderr, "Error opening %s\n", argv[1+i]);
            return EXIT_FAILURE;
        }
    }

    outputfp = fopen(argv[argc-1], "w+");
    if (!outputfp) {
        fprintf(stderr,"Error opening output %s\n", argv[argc-1]);
        return EXIT_FAILURE;
    }

    vector<thread> requesters, resolvers; 
    // make request threads
    for(int i = 0; i < numFiles; i++){
        requesters.emplace_back([i]{request(i);});
    }

    // resolver threads
    for(int i = 0; i < MAX_RESOLVER_THREADS; i++){
        resolvers.emplace_back([]{resolve();});
    }


    // Join requester and resolver threads
    for(auto &t : requesters)
        t.join();
    for(auto &t : resolvers)
        t.join();

    // Cleaning up
    queue_cleanup(&q);

    for (int i = 0; i < numFiles; i++){
        fclose(inputfp[i]);
    }

    fclose(outputfp);



    return EXIT_SUCCESS;
}

void queuePush(char* hostname){
    // queue must not be full, wait if it is
    unique_lock<mutex> lock(qMutex);
    notfull.wait(lock, []{return !queue_is_full(&q);} );

    if(queue_push(&q, hostname) == QUEUE_FAILURE){
        fprintf(stderr, "Queue push failed\n");
    }

    // unlocks lock and notify one thread that queue is not empty
    lock.unlock();
    notempty.notify_one();
}

char* queuePop(){
    unique_lock<mutex> lock(qMutex);
    notempty.wait(lock, []{return !queue_is_empty(&q) || liveProducers == 0;});

    // if empty, and not producers
    if(queue_is_empty(&q) && liveProducers == 0){
        return nullptr; 
    }

    char* host = (char*)queue_pop(&q);
    lock.unlock();
    notfull.notify_one();

    return host; 
}

void request(int fileI){
    FILE* fp = inputfp[fileI];
    char host[MAX_NAME_LENGTH];
    while (fscanf(fp, INPUTFS, host) > 0) {
        queuePush(strdup(host));
    }

    // Parethesises to make lock only lock and unlock in segment
    {
        // signal that this producer is done
        lock_guard<mutex> lg(qMutex);
        --liveProducers;
    }

    notempty.notify_all();
}

void resolve(){
    char ip[MAX_IP_LENGTH];
    while (1) {
        char* host = queuePop();
        if (!host) break;
        if (dnslookup(host, ip, sizeof(ip)) == EXIT_FAILURE) {
            fprintf(stderr, "dnslookup error: %s\n", host);
            ip[0] = '\0';
        }

        // Parethesises to make lock only lock and unlock in segment
        {
            // lock write to output file
            lock_guard<mutex> lg(qMutex);
            fprintf(outputfp, "%s,%s\n", host, ip);
            fflush(outputfp);
        }

        free(host);
    }
}