/*
    Create a TCP socket
*/
#include<stdio.h>
#include<winsock2.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
 
#pragma comment(lib,"ws2_32.lib") //Winsock Library
#define MAX_THREADS 100

typedef struct clientSocket {
	int id;
	int port;
	char *name;
	char *address;
	SOCKET sock;
	SOCKET senderPort;
} cSocket, *pcSocket;

HANDLE ghUpdateEvent;

BOOL set = FALSE;
char replied[2000][200];
int peerCount;
int receive(SOCKET s);
int pawnPeers(void);
int fillin();
pcSocket pcSocketArray[MAX_THREADS];
DWORD WINAPI MyThreadFunction( LPVOID lpParam );
DWORD WINAPI MyPeerThreadingFunction( LPVOID lpParam );

DWORD WINAPI UpdateList(LPVOID lpParam)
{
	pcSocket pcSocketStruct = (pcSocket)lpParam;
	int recv_size;
	char server_reply[200];
	
	while(TRUE)
	{
		sendMessage("someMessage",pcSocketStruct->sock);
		if((recv_size = recv(pcSocketStruct->sock , server_reply , 15 , 0)) == SOCKET_ERROR)
		{
			printf("Update receive Error : %d", WSAGetLastError());
		}
		//puts("In the update zone");
		server_reply[recv_size] = '\0';
		if(strcmp(server_reply,"update") == 0)
		{
			//puts("In the update zone1");
			sendMessage("someMessage",pcSocketStruct->sock);
			set = TRUE;
			if(receive(pcSocketStruct->sock) == 0)
				puts("Update ended");
		}
	}
	
	return 0;
}
 
int main(int argc , char *argv[])
{
	DWORD   dwThreadId;
    HANDLE  hThread;
	
    WSADATA wsa;
    SOCKET s[2];
    struct sockaddr_in server[2];
	char name[20000], *message;
	int copy = 0, clientCounter = 0;
	struct clientSocket pcStructure;
 
    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d\n",WSAGetLastError());
        return 1;
    }
     
    printf("Initialised.\n");
	
	puts("Connected\n\nEnter your name : ");
		
	fgets(name,2000,stdin);
    //Create two sockets
	
	ghUpdateEvent = CreateEvent( 
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        TEXT("Join Event")  // object name
        ); 

    if (ghUpdateEvent == NULL) 
    { 
        printf("CreateEvent failed (%d)\n", GetLastError());
        return;
    }
	for(int i = 0;i < 2; i++)
	{
		if((s[i] = socket(AF_INET , SOCK_STREAM ,IPPROTO_TCP)) == INVALID_SOCKET)
		{
			printf("Could not create socket : %d\n" , WSAGetLastError());
		}
	 
		printf("Socket created.\n");
		 
		server[i].sin_addr.s_addr = inet_addr("127.0.0.1");
		server[i].sin_family = AF_INET;
		server[i].sin_port = htons( 8888 );
	 
		//Connect to remote server
		if (connect(s[i] , (struct sockaddr *)&server[i] , sizeof(server[i])) < 0)
		{
			puts("connect error");
			return 1;
		}
		sendMessage(name,s[i]);
	}
	
	receive(s[0]);
	pcStructure.sock = s[1];
	
	hThread = CreateThread( 
            NULL,                   		// default security attributes
            0,                      		// use default stack size  
            UpdateList,       				// thread function name
            &pcStructure,				    // argument to thread function 
            0,                      		// use default creation flags 
            &dwThreadId);   				// returns the thread identifier
			
	if (hThread == NULL) 
    {
		puts("Thread not created at all at all");
    }
		//Receive a reply from the server
	closesocket(s[0]);
	//WSACleanup();
	pawnPeers();
	WaitForSingleObject(hThread,INFINITE);

    //WSACleanup();

 
    return 0;
}

int receive(SOCKET s)
{
	char server_reply[20000];
	char replies[2000][200];
	
	int recv_size,iterator=0;
	memset(&replies[0], 0, sizeof(replies));
	while(TRUE)
	{	
		if((recv_size = recv(s , server_reply , 15 , 0)) == SOCKET_ERROR)
		{
			printf("Receive Error : %d", WSAGetLastError());
		}
		//Add a NULL terminating character to make it a proper string before printing
		server_reply[recv_size] = '\0';
		if(strcmp(server_reply,"update") != 0)
		{
			if(strcmp(server_reply, "End") != 0)
			{
				strcpy(replies[iterator], server_reply);
				sprintf(replied[iterator],"%s",replies[iterator]);
				//printf("%s\n",replied[iterator]);
			}
			else{
				strcpy(replies[iterator], server_reply);
				sprintf(replied[iterator],"%s",replies[iterator]);
				//printf("%s\n",replied[iterator]);
				break;
			}
		}
		iterator++;
	}
	peerCount = iterator/4;
	
	return 0;
}

int sendMessage(char *message, SOCKET new_socket)
{
	int bytessent;
	if(bytessent = send(new_socket , message , strlen(message) , 0) == INVALID_SOCKET)
	{
		printf("Error : %d", WSAGetLastError());
		return 1;
	}
	return 0;
}

int pawnPeers(void)
{
	DWORD   dwThreadId,dwThreadId1[MAX_THREADS];
    HANDLE  hThread,hThread1[MAX_THREADS];
	
	SOCKET s;
	struct sockaddr_in server;
	int portint, count = 1,portindex,idint, answer = 0;
	DWORD dwWaitResult;
	struct clientSocket threadSock;
	
	pcSocketArray[0] = (pcSocket) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
	sizeof(struct clientSocket));
	
	portindex = (peerCount * 4) - 3;
	printf("Index : %d\n",portindex);
	portint= atoi(replied[portindex]);
	pcSocketArray[0]->port = portint;
	printf("%d\n",pcSocketArray[0]->port );
	
	hThread = CreateThread( 
            NULL,                   		// default security attributes
            0,                      		// use default stack size  
            MyThreadFunction,       		// thread function name
            pcSocketArray[0],			    // argument to thread function 
            0,                      		// use default creation flags 
            &dwThreadId);   	// returns the thread identifier
			
	if (hThread == NULL) 
    {
		puts("Thread not Created at all at all");
    }
		
	printf("Select a peer ID to talk to.\n");
	printf("Enter 0 to check and refresh list.\n");
	printf("Enter -1 to exit.\n******************************************\n");
	printf("%5s%10s%15s%15s\n", "ID","Port","Address","Name");
	fillin();	
	while(answer != -1)
	{
		printf("\nAnswer : ");		
		fflush(stdin);
		while(scanf("%d", &answer)< 1)
		{
			puts("Invalid answer. Try again.\n");
			printf("Answer : ");
		}	
		while(answer == 0)
		{
			puts("Filling in...");
			fillin();
			while(scanf("%d", &answer)< 1)
			{
				puts("Invalid answer. Try again.\n");
				printf("Answer : ");
			}		
		}
		
		//Create a socket
		if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
		{
			printf("Could not create socket : %d" , WSAGetLastError());
			break;
		}
	
		server.sin_addr.s_addr = inet_addr(pcSocketArray[answer]->address);
		server.sin_family = AF_INET;
		server.sin_port = htons( pcSocketArray[answer]->port);

		count=0;
		if(connect(s , (struct sockaddr *)&server , sizeof(server)) >= 0)
		{
			puts("Connected");
			threadSock.sock = s;
			threadSock.id = pcSocketArray[answer]->id;
			threadSock.port = pcSocketArray[answer]->port;
			threadSock.address = pcSocketArray[answer]->address;
			
			hThread1[count] = CreateThread( 
					NULL,                   		// default security attributes
					0,                      		// use default stack size  
					MyPeerThreadingFunction,       	// thread function name
					&threadSock,			    	// argument to thread function 
					0,                      		// use default creation flags 
					&dwThreadId1[count]);   		// returns the thread identifier
					
			if (hThread == NULL) 
			{
				puts("Thread not Created at all at all");
				break;
			}
			count++;
		}
		else{
			printf("Couldn't connect to peer. Error : %d",WSAGetLastError());
			break;
		}
	}
	
	WaitForMultipleObjects(count, hThread1, TRUE, INFINITE);
	
	for(int i=0; i<count; i++)
    {
        CloseHandle(hThread1[i]);
	}
	printf("Exited successfully");
	getchar();
	return 0;
}

int fillin()
{
	DWORD 	dwWaitResult;
	int iterator = 0, count = 1,portint,idint;
	while(count <= peerCount)
	{	
		free(pcSocketArray[count]);
		//printf("%s %s %s %s\n",replied[iterator],replied[iterator+1],replied[iterator+2],replied[iterator+3]);
		pcSocketArray[count] = (pcSocket) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			sizeof(struct clientSocket));
			
		idint = atoi(replied[iterator]);
		pcSocketArray[count]->id = idint;
		
		portint= atoi(replied[iterator+1]);
		pcSocketArray[count]->port = portint;
			
		pcSocketArray[count]->address = replied[iterator+2];
		pcSocketArray[count]->name = replied[iterator+3];
		
		if(pcSocketArray[count]->port != 0)
		{			
			printf("%5d%10d%15s%15s", count,pcSocketArray[count]->port ,pcSocketArray[count]->address,pcSocketArray[count]->name);
		}
		if(set)
		{
			iterator+=6;
		}
		else
		{
			iterator+=4;
		}
		count++;
	}
	
	return 0;
}

DWORD WINAPI MyThreadFunction( LPVOID lpParam ) 
{ 
	WSADATA wsa;
	SOCKET s,new_socket;
	struct clientSocket newClients[20];
	int c, bytessent, clientId = 0, recv_size;
	struct sockaddr_in server , client;
    char *message="",server_reply[20000];
	pcSocket pcSocketServer;
	
	pcSocketServer = (pcSocket)lpParam;
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d\n",WSAGetLastError());
        return 1;
    }

	if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
		return 1;
    }
	
	server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( pcSocketServer->port );
     
    //Bind
    if( bind(s ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d\n" , WSAGetLastError());
    }
	
	if(listen(s,3) != 0)
	{
		printf("Listen ERROR : %d\n", WSAGetLastError());
	}
	
	c = sizeof(struct sockaddr_in);

	while((new_socket = accept(s , (struct sockaddr *)&client, &c)) != INVALID_SOCKET)
	{
		sendMessage("Listening" , new_socket);
		
		server_reply[0] = '\0';
		if(recv_size = recv(new_socket , server_reply , 2000 , 0) == SOCKET_ERROR)
		{
			printf("Receive Error : %d\n", WSAGetLastError());
		}
		
		//server_reply[recv_size] = '\0';
		printf("Peer message : \"%s\"" , server_reply);
		//fflush(stdin);
		//server_reply[0] = '\0';
		sprintf(server_reply,"ACK from %d",pcSocketServer->port);
		sendMessage( server_reply ,new_socket);
	}
    return 0; 
} 

DWORD WINAPI MyPeerThreadingFunction( LPVOID lpParam ) 
{ 
	int recv_size,count = 1;
	char *message="", *port, server_reply[20000];
	DWORD dwWaitResult;
	SOCKET s;

	struct clientSocket threadSock;
	threadSock = *((pcSocket)lpParam);
	while(1==1)
	{
		strcpy(server_reply,"");
		if((recv_size = recv(threadSock.sock , server_reply , 2000 , 0)) != SOCKET_ERROR)
		{
			//Add a NULL terminating character to make it a proper string before printing
			//server_reply[recv_size] = '\0';
			printf("Peer says: %s\n" , server_reply);
			
			printf("\nEnter a message to send to client.\n Port : %d\t\tAddress : %s\n" ,threadSock.port, threadSock.address);
			fflush(stdin);
			fgets(message, 2000,stdin);
			
			sendMessage(message , threadSock.sock );
		}
	}
	return 0;
}