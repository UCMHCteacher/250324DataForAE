#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stdbool.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#elif _WIN32
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#endif


#ifdef _WIN32
#define CLOSE_SOCKET(s) closesocket(s)
#else
#define SOCKET int
#define CLOSE_SOCKET(s) close(s)
#endif


#include <thread>


#define PoolVolume 64

SOCKET receiveSocketPool[PoolVolume];
bool SocketIsUsed[PoolVolume];


void ReceiveThread(int socketNum) {
    int recvSocket = receiveSocketPool[socketNum];

    char buffer[4096];
    while (recv(recvSocket, buffer, 4096, 0) > 0) {}

    SocketIsUsed[socketNum] = false;
}




int main(int argc, char* argv[]) {
#ifdef _WIN32
    BYTE FirstVersion=0x02;
    BYTE SecondVersion=0x02;
    WORD RequestVersion=MAKEWORD(FirstVersion,SecondVersion);

    WSADATA StartupMessageGot;
    int StartupCondition;
    StartupCondition=WSAStartup(RequestVersion,&StartupMessageGot);

    if(StartupCondition!=0)
    {
        printf("Network startup failed!\n");
        printf("Error Code:%d\n",StartupCondition);
        return 0;
    }

    printf("Using socket version %d.%d\n",
            LOBYTE(StartupMessageGot.wVersion),
            HIBYTE(StartupMessageGot.wVersion)
    );
#endif





    // run socket on ipv4, stream-y, tcp
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // init addr
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(50000);
    serverAddr.sin_addr.s_addr = inet_addr(argv[1]);



    // bind socket to addr
    if (bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind failed");
        CLOSE_SOCKET(listenSocket);
        exit(EXIT_FAILURE);
    }



    if (listen(listenSocket, PoolVolume) < 0) {
        perror("listen failed");
        CLOSE_SOCKET(listenSocket);
        exit(EXIT_FAILURE);
    }



    while (true) {
        struct sockaddr_in clientAddress;
        int clientAddressLen;
        SOCKET tempReceiveSocket;

        tempReceiveSocket = accept(
            listenSocket,
            (struct sockaddr*)&clientAddress,
#ifdef _WIN32
            &clientAddressLen
#elif __linux__
            (unsigned int*)&clientAddressLen
#endif
        );

        if (tempReceiveSocket < 0) {
            perror("tempReceiveSocket foundation failed!\n");
            CLOSE_SOCKET(listenSocket);
            return 0;
        }
        printf("Received connection from %s:%d\n",
            inet_ntoa(clientAddress.sin_addr),
            ntohs(clientAddress.sin_port)
        );

        int ServerMessageSocketNumber=0;
        while(SocketIsUsed[ServerMessageSocketNumber]) ServerMessageSocketNumber++;


        receiveSocketPool[ServerMessageSocketNumber] = tempReceiveSocket;
        SocketIsUsed[ServerMessageSocketNumber]=true;


        std::thread t {ReceiveThread, ServerMessageSocketNumber};
        t.detach();
    }




    CLOSE_SOCKET(listenSocket);



#ifdef _WIN32
    // cleanups
    if(WSACleanup()!=0)
    {
        printf("Failed when closing network.\n");
        printf("Error Code:%d\n",WSAGetLastError());
    }
    printf("Successfully closed network.");
#endif


    return 0;
}