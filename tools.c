
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <sys/socket.h>

#include "tools.h"

// check if we can open the file
int fileLocked(char* path) {
	FILE* file = fopen(path, "r");
  	if (file) {
  		fclose(file);
      	return 0;
    }
  	return -1;
}

// return the type of the file
char* getType(char* subPath, int subPathLength) {
	// Get all text after the last period
	int typeLength = 0;
	char* type = (char*)malloc(subPathLength*sizeof(char));
	char cur = subPath[0];
	for (int i = 0; (i < subPathLength) && !(cur == ' ' || cur == EOF || cur == '\n' || cur == '\0' || cur == '\r'); i++) {
		type[typeLength++] = cur;

		// restart the type string if we see a period
		if (cur == '.') {
			free(type);
			type = (char*)malloc(subPathLength*sizeof(char));
			typeLength = 0;
		}

		cur = subPath[i+1];
	}
	type[typeLength] = '\0';

	return type;
}

// add the file type to the response
void catFileType(char** response, int* responseLength, int* maxResponseLength, char* type) {
	if (!strcmp(type, "html")) {
		catResponse(response, responseLength, maxResponseLength, HTML, HTML_LENGTH);
	} else if (!strcmp(type, "jpg")) {
		catResponse(response, responseLength, maxResponseLength, JPG, JPG_LENGTH);
	} else if (!strcmp(type, "css")) {
		catResponse(response, responseLength, maxResponseLength, CSS, CSS_LENGTH);
	} else if (!strcmp(type, "js")) {
		catResponse(response, responseLength, maxResponseLength, JAVASCRIPT, JAVASCRIPT_LENGTH);
	} else {
		catResponse(response, responseLength, maxResponseLength, DEFAULT_TYPE, DEFAULT_TYPE_LENGTH);
	}
	catResponse(response, responseLength, maxResponseLength, "\r\n", 2);
}

// add some text to the response
void catResponse(char** response, int* responseLength, int* maxResponseLength, char* toCat, int toCatLength) {
	*responseLength += toCatLength;

	if (*responseLength > *maxResponseLength) {
		while (*maxResponseLength < *responseLength) {
			*maxResponseLength *= 2;
		}
		*response = (char*)realloc(*response, *maxResponseLength*sizeof(char));
		assert(*response);
	}

	strcat(*response, toCat);
}

// read the next word into target starting from index
void setNextWord(char* line, int n, char* target, int* targetLength, int index) {
	char cur = line[index++];
	*targetLength = 0;
	while (index <= n && cur != ' ' && cur != '\0' && cur != EOF && cur != '\n') {
		target[(*targetLength)++] = cur;
		cur = line[index++];
	}
	target[*targetLength] = '\0';
}

// write the response to the client
void writeResponse(int sockfd, char* response, int responseLength) {
	int n = write(sockfd, response, responseLength);
	if (n < 0) {
		perror("write");
		exit(EXIT_FAILURE);
	}
}

// add body to response
int catBody(char** response, int* responseLength, int* maxResponseLength, char* path, int pathLength, bool isFull) {
	FILE* file = fopen(path, "rb");
	
	if (file == NULL) {
		return -1;
	}
	
	// get length of the file
	fseek(file, 0, SEEK_END);
	unsigned long fileLength = ftell(file);
	rewind(file);
	char* buffer = (char*)malloc(fileLength*sizeof(char));
	
	// read file content, close and update the response with the file's bytes
	fread(buffer, fileLength, sizeof(char), file);
	fclose(file);
	
	// cat the byte data to the response
	// up to 100000 chars in response
	if (isFull) {
		// catResponse(response, responseLength, maxResponseLength, CONTENT_LENGTH, CONTENT_LENGTH_LENGTH);
		char fileLengthChars[7];
		sprintf(fileLengthChars, "%ld", fileLength);
		// catResponse(response, responseLength, maxResponseLength, fileLengthChars, strlen(fileLengthChars));
		catResponse(response, responseLength, maxResponseLength, "\r\n", 2);
	}
	
	// adjust memory allocation for response according to file size
	*responseLength += fileLength;
	if (*responseLength >= *maxResponseLength) {
		while (*maxResponseLength < *responseLength) {
			*maxResponseLength *= 2;
		}
		*response = (char*)realloc(*response, *maxResponseLength*sizeof(char));
		assert(*response);
	}

	// add file data to response
	memcpy(*response + strlen(*response), buffer, fileLength);

	free(buffer);
	
	return 0;
}