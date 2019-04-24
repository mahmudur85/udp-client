#include <iostream>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>

using namespace std;

#define SERVER_ADDR "192.168.30.80"
#define PORT     5876
#define BUFSIZE 1400

static int make_socket_non_blocking (int sfd) {
    int flags, s;

    flags = fcntl (sfd, F_GETFL, 0);
    if (flags == -1)
    {
        perror ("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl (sfd, F_SETFL, flags);
    if (s == -1)
    {
        perror ("fcntl");
        return -1;
    }

    return 0;
}

static socklen_t get_address_len(const sockaddr* address) {
    if(address->sa_family == AF_INET) return sizeof(struct sockaddr_in);
    else if(address->sa_family == AF_INET6) return sizeof(struct sockaddr_in6);
    return 0;
}

// Driver code
int main() {
    int sockfd;
    char buffer[BUFSIZE];
    string hello("Hello from client");
    struct sockaddr_in     server_addr, local_addr;
    int optval = 1, n;
    socklen_t server_addr_len, local_addrLen;

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&server_addr, 0, sizeof(server_addr));

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    if(make_socket_non_blocking(sockfd) < 0){
        perror( "Could make socket non blocking");
        return EXIT_FAILURE;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(optval)) < 0) {
        perror("setsockopt() set SO_REUSEADDR");
        close(sockfd);
        return EXIT_FAILURE;
    }

    // Filling local_addr information
    bzero(&local_addr, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(60523);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    local_addrLen = get_address_len((const sockaddr*)&local_addr);

    if(bind(sockfd, (struct sockaddr *) &local_addr, local_addrLen) < 0){
        perror("peer bind()");
        close(sockfd);
        return EXIT_FAILURE;
    }

    // Filling server information
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    server_addr_len = get_address_len((const sockaddr*)&server_addr);

    if(connect(sockfd, (struct sockaddr *) &server_addr, server_addr_len) < 0){
        perror("peer connect()");
        close(sockfd);
        return EXIT_FAILURE;
    }

    /*sendto(sockfd, hello.c_str(), hello.length(),
           MSG_CONFIRM, (const struct sockaddr *) &server_addr,
           sizeof(server_addr));*/
    send(sockfd, hello.c_str(), hello.length(), MSG_DONTWAIT);
    printf("Hello message sent.\n");

//    n = (int) recvfrom(sockfd, (char *)buffer, BUFSIZE,
//                 MSG_WAITALL, (struct sockaddr *) &server_addr,
//                 (socklen_t *)&server_addr_len);
//    buffer[n] = '\0';
//    printf("Server : %s\n", buffer);

    close(sockfd);
    return 0;
}