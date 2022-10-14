#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h> 
#include "./ADMIN.h"
#include "./CUSTOMER.h"

void connection_handler(int connFd)
{
    printf("client is connected to the server\n");
    char rBuffer[1000], wBuffer[1000];
    ssize_t rBytes, wBytes;
    int choice;
    wBytes = write(connFd, "Welcome to bank!\nselect user\n1. admin\t2. customer\nPress any other number to exit\nEnter the number corresponding to the choice!", strlen("Welcome to bank!\nselect user\n1. admin\t2. customer\nPress any other number to exit\nEnter the number corresponding to the choice!"));
    if (wBytes == -1)
        perror("Error while sending message to the user!");
    else
    {
        bzero(rBuffer, sizeof(rBuffer));
        rBytes = read(connFd, rBuffer, sizeof(rBuffer));
        if (rBytes == -1)
            perror("Error while reading from client");
        else if (rBytes == 0)
            printf("No data was sent by the client");
        else
        {
            choice = atoi(rBuffer);
            switch (choice)
            {
            case 1:
                // Admin
                admin_operation(connFd);
                break;
            case 2:
                // Customer
                customer_operation(connFd);
                break;
            default:
                // Exit
                break;
            }
        }
    }
    printf("Terminating connection to client!\n");
}

void main()
{
    int socketFd, socketBindStatus, socketListenStatus, connFd;
    struct sockaddr_in serverAddress, clientAddress;

    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd == -1)
    {
        perror("Error while creating server socket!");
        _exit(0);
    }

    serverAddress.sin_family = AF_INET;                // IPv4
    serverAddress.sin_port = htons(8081);              // Server will listen to port 8080
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Binds the socket to all interfaces

    socketBindStatus = bind(socketFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (socketBindStatus == -1)
    {
        perror("Error while binding to server socket!");
        _exit(0);
    }

    socketListenStatus = listen(socketFd, 10);
    if (socketListenStatus == -1)
    {
        perror("Error while listening for connections on the server socket!");
        close(socketFd);
        _exit(0);
    }

    int clientSize;
    while (1)
    {
        clientSize = (int)sizeof(clientAddress);
        connFd = accept(socketFd, (struct sockaddr *)&clientAddress, &clientSize);
        if (connFd == -1)
        {
            perror("Error while connecting to client!");
            close(socketFd);
        }
        else
        {
            if (!fork())
            {
                // Child will enter this branch
                connection_handler(connFd);
                close(connFd);
                _exit(0);
            }
        }
    }

    close(socketFd);
}
