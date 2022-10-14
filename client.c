#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <string.h>

void connection_handler(int sockFD)
{
    char rBuffer[1000], wBuffer[1000]; // A buffer used for reading from / writting to the server
    ssize_t readBytes, writeBytes;            // Number of bytes read from / written to the socket

    char tBuffer[1000];

    do
    {
        bzero(rBuffer, sizeof(rBuffer)); // Empty the read buffer
        bzero(tBuffer, sizeof(tBuffer));
        readBytes = read(sockFD, rBuffer, sizeof(rBuffer));
        if (readBytes == -1)
            perror("Error while reading from client socket!");
        else if (readBytes == 0)
            printf("No error received from server! Closing the connection to the server now!\n");
        else if (strchr(rBuffer, '^') != NULL)
        {
            // Skip read from client
            strncpy(tBuffer, rBuffer, strlen(rBuffer) - 1);
            printf("%s\n", tBuffer);
            writeBytes = write(sockFD, "^", strlen("^"));
            if (writeBytes == -1)
            {
                perror("Error while writing to client socket!");
                break;
            }
        }
        else if (strchr(rBuffer, '$') != NULL)
        {
            // Server sent an error message and is now closing it's end of the connection
            strncpy(tBuffer, rBuffer, strlen(rBuffer) - 2);
            printf("%s\n", tBuffer);
            printf("Closing the connection to the server now!\n");
            break;
        }
        else
        {
            bzero(wBuffer, sizeof(wBuffer)); // Empty the write buffer

            if (strchr(rBuffer, '#') != NULL)
                strcpy(wBuffer, getpass(rBuffer));
            else
            {
                printf("%s\n", rBuffer);
                scanf("%[^\n]%*c", wBuffer); // Take user input!
            }

            writeBytes = write(sockFD, wBuffer, strlen(wBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing to client socket!");
                printf("Closing the connection to the server now!\n");
                break;
            }
        }
    } while (readBytes > 0);

    close(sockFD);
}
void main()
{
    int socketFd, connectStatus;
    struct sockaddr_in serverAddress;
    struct sockaddr server;

    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd == -1)
    {
        perror("Error while creating server socket!");
        _exit(0);
    }

    serverAddress.sin_family = AF_INET;                // IPv4
    serverAddress.sin_port = htons(8081);              // Server will listen to port 8080
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Binds the socket to all interfaces

    connectStatus = connect(socketFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (connectStatus == -1)
    {
        perror("Error while connecting to server!");
        close(socketFd);
        _exit(0);
    }

    connection_handler(socketFd);

    close(socketFd);
}

