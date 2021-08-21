#include "TCP_SERVER.h"

void main()
{
	time_t currentTime;

	// Initialize Winsock (Windows Sockets).
	WSAData wsaData;
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Time Server: Error at WSAStartup()\n";
		return;
	}

	// Create and bind a TCP socket.

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == listenSocket)
	{
		cout << "Time Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	sockaddr_in serverService;
	serverService.sin_family = AF_INET;
	serverService.sin_addr.s_addr = INADDR_ANY;
	serverService.sin_port = htons(TIME_PORT);
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *)&serverService, sizeof(serverService)))
	{
		cout << "Time Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	// Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(listenSocket, serverService, LISTEN);

	// Accept connections and handles them one by one.
	while (true)
	{
		//remove sockets that past timeout
		for (int i = 1; i < MAX_SOCKETS; i++)
		{
			currentTime = time(0);
			if ((sockets[i].recv != EMPTY || sockets[i].send != EMPTY) &&
				(sockets[i].lastUsed != 0) && (currentTime - sockets[i].lastUsed > 120))
			{
				cout << "Time Server: Client " << inet_ntoa(sockets[i].addr.sin_addr) << ":" << ntohs(sockets[i].addr.sin_port) << " has been disconected due to timeout." << endl;
				removeSocket(i);
			}
		}

		// The select function determines the status of one or more sockets,
		// waiting if necessary, to perform asynchronous I/O. Use fd_sets for
		// sets of handles for reading, writing and exceptions. select gets "timeout" for waiting
		// and still performing other operations (Use NULL for blocking). Finally,
		// select returns the number of descriptors which are ready for use (use FD_ISSET
		// macro to check which descriptor in each set is ready to be used).
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		//
		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.
		//
		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, &timeOut);
		/*select(int nfds, fd_set *read-fds, fd_set *write-fds, fd_set *except-fds, struct timeval *timeout)*/
		if (nfd == SOCKET_ERROR)
		{
			cout << "Time Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;

				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	//we will never get here because its a server...
	cout << "Time Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}
bool addSocket(SOCKET id, sockaddr_in socketAddr, eStatus what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			unsigned long flag = 1;
			if (ioctlsocket(id, FIONBIO, &flag) != 0)
			{
				cout << "Time Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
			}
			sockets[i].id = id;
			sockets[i].addr = socketAddr;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			sockets[i].lastUsed = time(0);
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	if(sockets[index].requesting!=nullptr)
	{
		if(sockets[index].requesting->resource_path!=nullptr)
		{
			delete[](sockets[index].requesting->resource_path);
		}
		if(sockets[index].requesting->accepted_language!=nullptr)
		{
			delete[](sockets[index].requesting->accepted_language);
		}
		if(sockets[index].requesting->acceptType!=nullptr)
		{
			delete[](sockets[index].requesting->acceptType);
		}
		if(sockets[index].requesting->body_message!=nullptr)
		{
			delete[](sockets[index].requesting->body_message);
		}
		delete(sockets[index].requesting);
	}
	closesocket(sockets[index].id);
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr *)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "Time Server: Error at accept(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		return;
	}
	cout << "Time Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	if (addSocket(msgSocket, from, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!" << endl;
		closesocket(msgSocket);
	}

	return;
}
void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "Time Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "Time Server: Recieved: " << bytesRecv << " bytes of " << endl;
		cout << "\"" << &sockets[index].buffer[len] << "\"" << endl << "message." << endl;

		sockets[index].len += bytesRecv;

		if (sockets[index].len > 0)
		{
			sockets[index].requesting = new request_info[1];
			intitiall(sockets[index].requesting,index);
			fillRequestHeaderMethod(sockets[index].requesting, &sockets[index].buffer[len],index);
			fillheadersandbody(sockets[index].requesting, &sockets[index].buffer[len]);
			cout << sockets[index].requesting->contentType << endl;
			cout << sockets[index].requesting->acceptType << endl;
			cout << sockets[index].requesting->accepted_language << endl;
			cout << sockets[index].requesting->body_message << endl;
			cout << sockets[index].requesting->connectType << endl;
			cout << sockets[index].requesting->resource_path << endl;
			cout << sockets[index].requesting->content_length << endl;
			cout << sockets[index].requesting->version << endl;

		}
	}
}
void intitiall(request_info*requ,int index)
{
	
	requ->resource_path = nullptr;
	requ->content_length = 0;
	requ->host = nullptr;
	requ->accepted_language = nullptr;
	requ->contentType = GET;
	requ->body_message = nullptr;
	requ->version = nullptr;

}
void fillRequestHeaderMethod(request_info*requ,char*buff,int index)
{
	istringstream resp(buff);
	string header;
	char method[15];
	getline(resp, header);
	int indexes = header.find(' ');
	int i = 0;
	while(i<indexes)
	{
		method[i] = header[i];
		i++;
	}
	method[i] = '\0';
	i = 0;
	if(strncmp(method,"GET",3)==0)
	{
		requ->contentType = GET;
		sockets[index].send = SEND;

	}
	if(strncmp(method,"POST",4)==0)
	{
		requ->contentType = POST;
		sockets[index].send = SEND;
	}
	if(strncmp(method,"HEAD",4)==0)
	{
		requ->contentType = HEAD;
		sockets[index].send = SEND;
	}
	if(strncmp(method,"PUT",3)==0)
	{
		requ->contentType = PUT;
		sockets[index].send = SEND;
	}
	if(strncmp(method,"DELETE",6)==0)
	{
		requ->contentType = _DELETE;
		sockets[index].send = SEND;

	}
	if(strncmp(method,"OPTIONS",7)==0)
	{
		requ->contentType = OPTIONS;
		sockets[index].send = SEND;
	}
	if(strncmp(method,"TRACE",5)==0)
	{
		requ->contentType = TRACE;
		sockets[index].send = SEND;
	}
	char resorch[250];
	requ->resource_path = new char[200];
	int indexes2 = header.find(' ', indexes + 1);
	int indexesslesh = header.find('/');
	i = indexesslesh+1;
	int p = 0;
	while(i<indexes2)
	{
		resorch[p] = header[i];
		p++;
		i++;
	}
	resorch[p] = '\0';
	strcpy(requ->resource_path, resorch);
	requ->version = new char[15];
	int j = indexes2 + 1;
	p = 0;
	while(header[j]!='\0')
	{
		requ->version[p] = header[j];
		p++;
		j++;
	}
	requ->version[p] = '\0';
	p = 0;


}
void fillheadersandbody(request_info*requ,char*buff)
{
	bool found = false;
	int count = countLines(buff);
	int indexLine = 0;
	istringstream resp(buff);
	string header;
	char str[100];
	char str2[100];
	int i = 0;
	int p = 0;
	int j;
	requ->acceptType = new char[100];
	requ->acceptType[0] = '\0';
	requ->accepted_language = new char[30];
	requ->accepted_language[0] = '\0';
	requ->body_message = new char[1000];
	requ->body_message[0] = '\0';
	getline(resp, header);
	indexLine = 1;
   while(indexLine<=count&&header!="\r")
   {
	   i = 0;
	   int indexes = header.find(':');
	   while(i<indexes)
	   {
		   str[i] = header[i];
		   i++;
	   }
	   str[i] = '\0';
	   if(strncmp(str,"Connection",10)==0)
	   {
		   j = indexes + 2;
		   p = 0;
		   while(header[j]!='\0')
		   {
			   str2[p] = header[j];
			   p++;
			   j++;
		   }
		   str2[j] = '\0';
		   j = 0;
		   if(strncmp(str2,"keep-alive",10)==0)
		   {
			   requ->connectType = keep_alive;
		   }
		   if(strncmp(str2,"close",5)==0)
		   {
			   requ->connectType = close;
		   }
		   str2[0] = '\0';

	   }
	   if(strcmp(str,"Accept")==0)
	   {
		   int indexescomma = header.find(',');
		   j = indexes + 2;
		   p = 0;
		   while(j<indexescomma)
		   {
			   str2[p] = header[j];
			   p++;
			   j++;
		   }
		   str2[p] = '\0';
		   strcpy(requ->acceptType, str2);
		   str2[0] = '\0';
	   }
	   if(strcmp(str,"Content-Length")==0)
	   {
		   char buffint[12];
		   buffint[0] = '\0';
		   j = indexes + 2;
		   p = 0;
		   while((header[j]!='\r')&&(header[j]!='\0'))
		   {
			   buffint[p] = header[j];
			   p++;
			   j++;
		   }
		   buffint[p] = '\0';
		   requ->content_length = ConvertStringToInt(buffint);
		   str2[0] = '\0';
		   buffint[0] = '\0';

	   }
	   if(strncmp(str,"Accept-Language",15)==0)
	   {
		   int indexcomma = header.find(',');
		   int indexcomma2 = header.find(';');
		   j = indexcomma + 1;
		   p = 0;
		   while(j<indexcomma2)
		   {
			   str2[p] = header[j];
			   p++;
			   j++;
		   }
		   str2[p] = '\0';
		   strcpy(requ->accepted_language, str2);
		   str2[0] = '\0';

	   }
	   str[0] = '\0';
	   indexLine++;
	   getline(resp, header);

   }
	if(indexLine==count+1)
	{
		requ->body_message[0] = '\0';
	}
	else
	{
		getline(resp, header);
		indexLine++;
		i = 0;
		p = 0;
		while(indexLine<=count)
		{
			if(header[i]!='\0')
			{
				requ->body_message[p] = header[i];
				p++;
				i++;
			}
			else
			{
				requ->body_message[p] = '\n';
				p++;
				getline(resp, header);
				indexLine++;
				i = 0;

			}
		}
		requ->body_message[p] = '\0';
	}
	
}
int countLines(char*buff)
{
	istringstream resp(buff);
	string header;
	int count = 0;
	while(getline(resp,header))
	{
		count++;
	}
	return count;
}
int ConvertStringToInt(char str[])
{
	int i = 0;
	int count = strlen(str);
	int sum = 0;
	while(i<count)
	{
		sum = sum * 10 + str[i] - '0';
		i++;
	}
	return sum;
}