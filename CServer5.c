/*
    Bind socket to port 8888 on localhost
*/
//#include<io.h>
#include<stdio.h>
#include<winsock2.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
 
#pragma comment(lib,"ws2_32.lib") //Winsock Library
 
#define MAX_THREADS 10
#define BUF_SIZE 255

HANDLE ghJoinEvent; 
DWORD WINAPI clientHandler( LPVOID lpParam );
int sendMessage(char *message, SOCKET new_socket);

//To be used for the list and to the clientHandler thread 
typedef struct clientSocket {
	int id;
	int port;
	char *address;
	char name[20000];
	SOCKET sock;
} cSocket, *pcSocket;

//Global list of clients for all threads
pcSocket pcSocketArray[MAX_THREADS];
int stop = 0;
 
int main(int argc , char *argv[])
{
    DWORD   dwThreadIdArray[MAX_THREADS];
    HANDLE  hThreadArray[MAX_THREADS]; 
	
    WSADATA wsa;
    SOCKET s , new_socket;
    struct sockaddr_in server , client;
    int i,c, clientId = 0, iterator, recv_size;
    char *localIP, *message;	
	char *name="", server_reply[20000];

 
    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d",WSAGetLastError());
        return 1;
    }
     
    printf("Initialised.\n");
	
	ghJoinEvent = CreateEvent( 
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        TEXT("Join Event")  // object name
        ); 

    if (ghJoinEvent == NULL) 
    { 
        printf("CreatJoinEvent failed (%d)\n", GetLastError());
        return;
    }
     
    //Create a socket
    if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
		return 1;
    }
 
    printf("Socket created.\n");
	
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
     
    //Bind
    if( bind(s ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d" , WSAGetLastError());
    }
     
    puts("Bind done");
 
    //Listen to incoming connections
    listen(s , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
     
    c = sizeof(struct sockaddr_in);
	
	printf("****************************************\n\n");
	printf("%5s%15s%15s%15s\n", "ID","Port" , "Address", "Name");
	
    while((new_socket = accept(s , (struct sockaddr *)&client, &c)) != INVALID_SOCKET )
    {
		//Receive initial message
		if(recv_size = recv(new_socket , server_reply , sizeof(server_reply) , 0) == SOCKET_ERROR)
		{
			printf("Receive error : %d" , WSAGetLastError());
			return 1;
		}
		server_reply[2000] = '\0';
		pcSocketArray[clientId] = (pcSocket) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			sizeof(struct clientSocket));
							
		if(pcSocketArray[clientId] == NULL)
		{
			puts("No memory for client info.\n");
			return 1;
		}
							
		//Add accepted socket to list of connected sockets
		pcSocketArray[clientId]->id = clientId;
 		pcSocketArray[clientId]->port = ntohs(client.sin_port);
		pcSocketArray[clientId]->address = inet_ntoa(client.sin_addr);
		pcSocketArray[clientId]->sock = new_socket;
		strcpy(pcSocketArray[clientId]->name,server_reply);
		
		if(clientId % 2 == 0)
		{
			printf("%5d%15d%15s%15s",(pcSocketArray[clientId]->id)/2 ,pcSocketArray[clientId]->port, pcSocketArray[clientId]->address,pcSocketArray[clientId]->name);
			if (!PulseEvent(ghJoinEvent)) 
			{
				printf("SetEvent failed (%d)\n", GetLastError());
			}
		}
		//Create the thread to begin execution on its own.
        hThreadArray[clientId] = CreateThread( 
            NULL,                   		// default security attributes
            0,                      		// use default stack size  
            clientHandler,       			// thread function name
            pcSocketArray[clientId],        // argument to thread function 
            0,                      		// use default creation flags 
            &dwThreadIdArray[clientId]);   	// returns the thread identifier 
			
		if (hThreadArray[clientId] == NULL) 
        {
           puts("Error here");
           //return 1;
		}
		clientId++;
    }
	
	 // Wait until all threads have terminated.
	WaitForMultipleObjects(MAX_THREADS, hThreadArray, TRUE, INFINITE);
	
	for(int i=0; i<MAX_THREADS; i++)
    {
        CloseHandle(hThreadArray[i]);
        if(pcSocketArray[i] != NULL)
        {
            HeapFree(GetProcessHeap(), 0, pcSocketArray[i]);
            pcSocketArray[i] = NULL;    // Ensure address is not reused.
        }
    }

	printf("****************************************\n\n");
	
    getchar();
 
    closesocket(s);
    WSACleanup();
     
    return 0;
}


DWORD WINAPI clientHandler( LPVOID lpParam ) 
{ 
    pcSocket pcSentSocket;
	DWORD dwWaitResult;
	int iterator,recv_size;
	char *port = "",*id="";
	char server_reply[200];
	
	
	// Cast the parameter to the correct data type.
    // The pointer is known to be valid because 
    // it was checked for NULL before the thread was created.
 
    pcSentSocket = (pcSocket)lpParam;
	
    // Print the parameter values using thread-safe functions.

	for(iterator = 0; iterator < MAX_THREADS ; iterator++)
	{
		if(pcSocketArray[iterator] != NULL)
		{
			// Send initial message
			if(iterator % 2 == 0)
			{
				sprintf(id, "%d" , pcSocketArray[iterator]->id);	
				sendMessage(id, pcSentSocket->sock);	
				Sleep(200);
				sprintf(port, "%d" , pcSocketArray[iterator]->port);	
				sendMessage(port, pcSentSocket->sock);	
				Sleep(200);
				sendMessage(pcSocketArray[iterator]->address, pcSentSocket->sock);
				Sleep(200);
				sendMessage(pcSocketArray[iterator]->name, pcSentSocket->sock);
				Sleep(200);
			}
		}
	}
	if(pcSentSocket->id % 2 == 0)
	{
		sendMessage("End" , pcSentSocket->sock);
		Sleep(500);
	}
	else{
		while(TRUE){
			dwWaitResult = WaitForSingleObject( 
			ghJoinEvent, // event handle
			INFINITE);    // indefinite wait
			
			if (dwWaitResult  == WAIT_OBJECT_0)
			{
				puts("Join event Triggered.");
				Sleep(200);
				if(recv_size = recv(pcSentSocket->sock , server_reply , sizeof(server_reply) , 0) == SOCKET_ERROR)
				{
					printf("Receive error : %d" , WSAGetLastError());
					return 1;
				}
				else{
					puts("updating....");
				}
	
				for(iterator = 0; iterator < MAX_THREADS ; iterator++)
				{
					if(pcSocketArray[iterator] != NULL)
					{
						// Send initial message
						sendMessage("update", pcSentSocket->sock);
						Sleep(200);
						if(iterator % 2 == 0)
						{
							sprintf(id, "%d" , pcSocketArray[iterator]->id);	
							sendMessage(id, pcSentSocket->sock);	
							Sleep(200);
							sprintf(port, "%d" , pcSocketArray[iterator]->port);	
							sendMessage(port, pcSentSocket->sock);	
							Sleep(200);
							sendMessage(pcSocketArray[iterator]->address, pcSentSocket->sock);
							Sleep(200);
							sendMessage(pcSocketArray[iterator]->name, pcSentSocket->sock);
							Sleep(200);
						}
					}
				}
				sendMessage("End" , pcSentSocket->sock);
				Sleep(200);
			}
		}
	}

    return 0; 
} 

int sendMessage(char *message, SOCKET new_socket)
{
	int bytessent;
	if(bytessent = send(new_socket , message , strlen(message) , 0) == INVALID_SOCKET)
	{
		printf("Error sending: %d", WSAGetLastError());
		return 1;
	}
	return 0;
}