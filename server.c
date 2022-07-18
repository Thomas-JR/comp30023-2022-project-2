#define _POSIX_C_SOURCE 200112L
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <limits.h>

#include "tools.h"
#include "response.c"

// specification requests
#define IMPLEMENTS_IPV6
#define MULTITHREADED

// ========== MAIN FUNCTION ==========
int main(int argc, char** argv) {
	int sockfd, re, s;
	struct addrinfo hints, *res;

	if (argc != 3 && argc != 4) {
		fprintf(stderr, "ERROR, invalid input\n");
		exit(EXIT_FAILURE);
	}

	char* protocolNumber = argv[1];
	char* portNumber = argv[2];
	char* rootDirectory = argc == 4 ? argv[3] : "";

	// Create address we're going to listen on (with given port number)
	memset(&hints, 0, sizeof hints);
	if (!strcmp(protocolNumber, "4")) {
		// IPv4
		hints.ai_family = AF_INET;
	} else if (!strcmp(protocolNumber, "6")) {
		// IPv6
		hints.ai_family = AF_INET6;
	} else {
		perror("invalid protocol number");
		exit(EXIT_FAILURE);
	}
	
	hints.ai_socktype = SOCK_STREAM;	// TCP
	hints.ai_flags = AI_PASSIVE;     	// for bind, listen, accept

	// node (NULL means any interface), service (port), hints, res
	s = getaddrinfo(NULL, portNumber, &hints, &res);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	// Create socket
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Reuse port if possible
	// CHECK: does this work for multiple sockets
	re = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	// Prevent timeout
	int enable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		perror("setsockopt");
		exit(1);
	}

	// Bind address to the socket
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(res);

	// setup lots of threads which we can add to the list
	// int maxThreadCount = MAX_THREAD_COUNT;
	// int threadCount = 0;
	pthread_mutex_t lock;

	// setup thread locking
	if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("mutex initialistion failed\n");
        return 1;
    }

	// while the server is running...
	while (1) {
		// listen for new socket connection attempts
		printf("listening\n");
		if (listen(sockfd, MAX_LISTEN_COUNT) < 0) {
			perror("listen");
			exit(EXIT_FAILURE);
		}
		
		struct sockaddr_storage clientAddress;
		socklen_t clientAddressSize = sizeof clientAddress;
		
		// accept the client connection
		int newsockfd = accept(sockfd, (struct sockaddr*)&clientAddress, &clientAddressSize);
		if (newsockfd < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		
		// construct the data to be passed into the thread
		struct thread_data* data = (struct thread_data*)malloc(1*sizeof(struct thread_data));
		data->sockfd = newsockfd;
		data->rootDirectory = rootDirectory;
		data->lock = &lock;
		
		// create the thread
		
		pthread_t thread = (pthread_t)malloc(1*sizeof(pthread_t));
		if (pthread_create(&thread, NULL, socket_thread, (void*)data)) {
			perror("thread creation failure\n");
			return 1;
		}
		
		printf("getting next socket\n\n");
	}
	
	close(sockfd);
	
	return 0;
}