#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#include <iostream>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include<stdlib.h>
#include<stdio.h>
#include <string>
#include <fstream>
#include <sstream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
typedef enum Connect{
	keep_alive=1,
	close=2
	} Connect;
typedef enum eMethod {
	GET = 1,
	POST=2,
	HEAD = 3,
	PUT = 3,
	_DELETE = 4,
	TRACE = 5,
	OPTIONS = 6
} eMethod;
struct request_info
{
	char* resource_path;
	int content_length;
	char*host;
	char*accepted_language;
	eMethod contentType;
	Connect connectType;
	char*body_message;
	char*acceptType;
	char*version;


};
struct response_info
{
	char*status_code;
	char*date;
	char *connection;
	int age;
	int content_length;
	char *last_modified;
	char*version;
};
typedef enum eStatus {
	EMPTY = 0,
	LISTEN = 1,
	RECEIVE = 2,
	IDLE = 3,
	SEND = 4,
} eStatus;


struct SocketState
{
	request_info*requesting;
	response_info*responsing;
	SOCKET id;				// Socket handle
	sockaddr_in addr;
	eStatus	recv;			// Receiving?
	eStatus	send;	// Sending?	// Sending sub-type
	eMethod sendSubType;
	char buffer[10000];
	int len;
	time_t lastUsed;
};
const int MAX_SOCKETS = 5;
const int TIME_PORT = 27016;
struct SocketState sockets[MAX_SOCKETS] = { 0 }; //global dew lazyness
int socketsCount = 0;
struct timeval timeOut;
void acceptConnection(int index);
bool addSocket(SOCKET id, sockaddr_in socketAddr, eStatus what);
void receiveMessage(int index);
void removeSocket(int index);
void intitiall(request_info*requ,int index);
void fillRequestHeaderMethod(request_info*requ, char*buff, int index);
void fillheadersandbody(request_info*requ, char*buff);
int countLines(char*buff);
int ConvertStringToInt(char str[]);

