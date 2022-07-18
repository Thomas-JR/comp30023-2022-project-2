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

struct thread_data {
	int sockfd;
	char* rootDirectory;
	pthread_mutex_t* lock;
};

// main socket function - tries to construct a response and send it based on the connection's requests
void* socket_thread(void* data) {
	int n;
	char buffer[MAX_BUFFER_SIZE+1];
	memset(buffer, 0, MAX_BUFFER_SIZE);
	
	struct thread_data curData = *(struct thread_data*)data;
	int sockfd = curData.sockfd;
	char* rootDirectory = curData.rootDirectory;
	pthread_mutex_t* lock = curData.lock;

	int requestLength = 0;
	char* request = (char*)malloc(MAX_BUFFER_SIZE*sizeof(char));

	pthread_mutex_lock(lock);
	
	// Read characters from the connection, then process
	while ((n = recv(sockfd, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
		if (n < 0) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		memcpy(request + requestLength, buffer, n);
		requestLength += n;

		if ((strstr(buffer, "\r\n\r\n") != NULL)) {
			break;
		}

	}

	request[requestLength] = '\0';

	// Store the HTTP method type
	int methodLength = 0;
	char* method = (char*)malloc(MAX_METHOD_LENGTH*sizeof(char));
	setNextWord(buffer, n, method, &methodLength, 0);

	// Respond to request
	if (!strcmp(method, "GET") && (n-14) <= PATH_MAX) {
		int pathLength = 0;
		char path[PATH_MAX+1] = "";
		strcat(path, rootDirectory);
		pathLength += strlen(rootDirectory);

		// get the sub path from the request file
		int subPathLength = 0;
		char subPath[PATH_MAX+1];
		int secondWordStart = -1;

		for (int i = 0; i < n; i++) {
			if (buffer[i] == ' ') {
				secondWordStart = i+1;
				break;
			}
		}
		setNextWord(buffer, n, subPath, &subPathLength, secondWordStart);
		subPath[subPathLength++] = '\0';
		bool pathEscape = false;
		for (int i = 0; i < subPathLength; i++) {
			if (i < subPathLength-2 && subPath[i] == '.' && subPath[i+1] == '.' && subPath[i+2] == '/') {
				pathEscape = true;
				break;
			}
		}

		// concatenate the sub path to the file path
		pathLength += subPathLength;
		strcat(path, subPath);

		int versionLength;
		char version[n];
		setNextWord(buffer, n, version, &versionLength, GET_SPACING + subPathLength);
		bool isFull = versionLength > 1 ? true : false;

		printf("Path: %s\n", path);

		// Write success status if the file is accessable from the path
		if (!pathEscape && !access(path, F_OK) && !access(path, R_OK) && !fileLocked(path)) {
			int maxResponseLength = 128;
			int responseLength = 0;
			char* response = (char*)malloc(maxResponseLength*sizeof(char));

			if (isFull) {
				// Construct status line
				catResponse(&response, &responseLength, &maxResponseLength, SUCCESS_STATUS, SUCCESS_STATUS_LENGTH);

				// Construct response header
				// Add Content-Type to response
				catResponse(&response, &responseLength, &maxResponseLength, CONTENT_TYPE, CONTENT_TYPE_LENGTH);
				int subPathLength = 0;
				char subPath[PATH_MAX];
				setNextWord(buffer, n, subPath, &subPathLength, GET_SPACING);
				char* fileType = (char*)malloc(n*sizeof(char));
				fileType = getType(subPath, subPathLength);
				catFileType(&response, &responseLength, &maxResponseLength, fileType);
				free(fileType);
			}

			// Add body to response if we can
			if (catBody(&response, &responseLength, &maxResponseLength, path, pathLength, isFull)) {
				writeResponse(sockfd, ERROR_STATUS, ERROR_STATUS_LENGTH);
			} else {
				writeResponse(sockfd, response, responseLength);
			}
			free(response);
		} else {
			printf("bad path\n");
			writeResponse(sockfd, ERROR_STATUS, ERROR_STATUS_LENGTH);
		}
	} else if (strcmp(method, "POST")) {
	} else if (strcmp(method, "PUT")) {
	} else if (strcmp(method, "PATCH")) {
	} else if (strcmp(method, "DELETE")) {
	} else {
		printf("returning error\n");
		writeResponse(sockfd, METHOD_ERROR, METHOD_ERROR_LENGTH);
	}

	free(method);

	pthread_mutex_unlock(lock);
	
	close(sockfd);

	printf("closing connection\n");

	pthread_exit(0);
}

// ./client 172.26.132.190 8080
// GET /subdir/other.html HTTP/1.0
// GET /www/assets/image.jpg HTTP/1.0
