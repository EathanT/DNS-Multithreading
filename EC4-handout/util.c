/*
 * File: util.c
 * Author: Andy Sayler
 * Project: CSCI 3753 Programming Assignment 2
 * Create Date: 2012/02/01
 * Modify Date: 2012/02/01
 * Description:
 * 	This file contains declarations of utility functions for
 *      Programming Assignment 2.
 *  
 */

#include "util.h"

// Improved DNS lookup function
int dnslookup(const char* hostname, char* ip, int maxSize){
    /* Local vars */
    struct addrinfo* headresult = NULL;
    struct addrinfo* result = NULL;

    char ipv4str[INET_ADDRSTRLEN];
    struct sockaddr_in* ipv4sock ;
    struct in_addr* ipv4addr;

    char ipv6str[INET6_ADDRSTRLEN];
    struct sockaddr_in6* ipv6sock;
    struct in6_addr* ipv6addr;

    char ipstr[INET6_ADDRSTRLEN]; 
    int addrError = 0;

    /* DEBUG: Print Hostname*/
    if(VERBOSE) fprintf(stderr, "%s\n", hostname);
   
    /* Lookup Hostname */
    addrError = getaddrinfo(hostname, NULL, NULL, &headresult);
    if(addrError){
	    fprintf(stderr, "Error looking up Address: %s\n",
		gai_strerror(addrError));
	    return EXIT_FAILURE;
    }

    /* Loop Through result Linked List */
    for(result = headresult; result != NULL; result = result->ai_next){
	    /* Extract IP Address and Convert to String */
	    if(result->ai_addr->sa_family == AF_INET){
                /* IPv4 Address Handling */
                ipv4sock = (struct sockaddr_in*)(result->ai_addr);
                ipv4addr = &(ipv4sock->sin_addr);
                if(!inet_ntop(result->ai_family, ipv4addr, ipv4str, sizeof(ipv4str))){
                    perror("Error Converting IP to String at IPv4");
                    return EXIT_FAILURE;
                }
        if(VERBOSE) fprintf(stdout, "%s\n", ipv4str);
        
        strncpy(ipstr, ipv4str, sizeof(ipstr));
        ipstr[sizeof(ipstr)-1] = '\0';
	}
	else if(result->ai_addr->sa_family == AF_INET6){
        /* IPv6 Address Handling */
        ipv6sock = (struct sockaddr_in6*)(result->ai_addr);
        ipv6addr = &(ipv6sock->sin6_addr);
        if(!inet_ntop(result->ai_family, ipv6addr,ipv6str, sizeof(ipv6str))){
            perror("Error Converting IP to String at IPv6");
        return EXIT_FAILURE;
        }
        if(VERBOSE) fprintf(stdout, "%s\n", ipv6str);
    
        strncpy(ipstr, ipv6str, sizeof(ipstr));
        ipstr[sizeof(ipstr)-1] = '\0';
	}
	else{
	    /* Unhandlded Protocol Handling */
	    if(VERBOSE)fprintf(stdout, "Unknown Protocol: Not Handled\n");
	    strncpy(ipstr, "UNHANDELED", sizeof(ipstr));
	    ipstr[sizeof(ipstr)-1] = '\0';
	}
	/* Save First IP Address */
	if(result==headresult){
	    strncpy(ip, ipstr, maxSize);
	    ip[maxSize-1] = '\0';
	}
    }

    /* Cleanup */
    freeaddrinfo(headresult);

    return EXIT_SUCCESS;
}


int get_core_count(){
    return sysconf(_SC_NPROCESSORS_ONLN);
}