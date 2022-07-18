#ifndef MY_HEADER_FILE_H
#define MY_HEADER_FILE_H

// type constants
#define HTML "text/html"
#define HTML_LENGTH 9
#define JPG "image/jpeg"
#define JPG_LENGTH 10
#define CSS "text/css"
#define CSS_LENGTH 8
#define JAVASCRIPT "text/javascript"
#define JAVASCRIPT_LENGTH 15
#define DEFAULT_TYPE "application/octet-stream"
#define DEFAULT_TYPE_LENGTH 24

// response statuses
#define ERROR_STATUS "HTTP/1.0 404 Not Found\r\n\r\n"
#define ERROR_STATUS_LENGTH 24
#define SUCCESS_STATUS "HTTP/1.0 200 OK\r\n"
#define SUCCESS_STATUS_LENGTH 17

// header statuses
#define TRANSFER_ENCODING "Transfer-Encoding: chunked\n"
#define TRANSFER_ENCODING_LENGTH 27
#define CONTENT_TYPE "Content-Type: "
#define CONTENT_TYPE_LENGTH 14
#define CONTENT_LENGTH "Content-Length: "
#define CONTENT_LENGTH_LENGTH 16
#define METHOD_ERROR "HTTP ERROR: INVALID METHOD\r\n"
#define METHOD_ERROR_LENGTH 28

// other constants
#define GET_SPACING 4
#define MAX_METHOD_LENGTH 6
#define MAX_LISTEN_COUNT 100
#define MAX_BUFFER_SIZE 2048
#define MAX_THREAD_COUNT 128

char* getType(char* subPath, int subPathLength);
int fileLocked(char* path);
void catFileType(char** response, int* responseLength, int* maxResponseLength, char* type);
void catResponse(char** response, int* responseLength, int* maxResponseLength, char* toCat, int toCatLength);
void writeResponse(int newsockfd, char* response, int responseLength);
void setNextWord(char* buffer, int n, char* target, int* targetLength, int index);
int catBody(char** response, int* responseLength, int* maxResponseLength, char* path, int pathLength, bool isFull);

#endif