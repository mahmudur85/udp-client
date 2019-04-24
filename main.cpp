#include <iostream>
#include <string>
#include <thread>
#include <csignal>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>

using namespace std;

#define SERVER_ADDR "192.168.30.80"
#define PORT     5876
#define BUFSIZE 1400

static bool running = true;
static int sockfd;

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

/**
 * We will register SIGINT, since we cannot register SIGKILL and SIGSTOP
 */
void sig_handler(int signo) {
    if (signo == SIGINT)
        cout << "received SIGINT" << endl;
    if (signo == SIGABRT)
        cout << "received SIGABRT" << endl;
    if (signo == SIGSEGV)
        cout << "received SIGSEGV" << endl;

    cout << "Shutdown Initiating" << endl;

    cout << "Shutdown Complete" << endl;
    exit(1);
}

void register_signals() {
    std::cout << "Registering  Signals" << std::endl;
    if (signal(SIGINT, sig_handler) == SIG_ERR)
        cout << "Not able to register SIGNALS (SIGINT)" << endl;
    if (signal(SIGABRT, sig_handler) == SIG_ERR)
        cout << "Not able to register SIGNALS (SIGABRT)" << endl;
    if (signal(SIGSEGV, sig_handler) == SIG_ERR)
        cout << "Not able to register SIGNALS (SIGSEGV)" << endl;

    running = false;

    close(sockfd);
}

static socklen_t get_address_len(const sockaddr* address) {
    if(address->sa_family == AF_INET) return sizeof(struct sockaddr_in);
    else if(address->sa_family == AF_INET6) return sizeof(struct sockaddr_in6);
    return 0;
}

void send_thread(int fd){
    char buffer[256];
    struct timeval tv;
    unsigned long time_in_micros;
    for(int i = 1; i <= 10; i++ ){
        memset(buffer,0, sizeof(buffer));
        gettimeofday(&tv,NULL);
        time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
        sprintf(buffer, "%lld", (long long) time_in_micros);
        send(fd, buffer, strlen(buffer), MSG_DONTWAIT);
        cout << "\nsent >> " << buffer << endl;
        usleep(1);
    }
}

void recv_thread(int fd, struct sockaddr *server_addr, socklen_t *server_addr_len){
    char buffer[BUFSIZE];
    while(running) {
        while (true) {
            auto r = (int) recv(fd, buffer, BUFSIZE, MSG_DONTWAIT);
            /*auto r = (int) recvfrom(fd, (char *) buffer, BUFSIZE,
                                    0, server_addr,
                                    server_addr_len);*/
            if (r == -1) {
                if ((errno == EAGAIN) ||
                    (errno == EWOULDBLOCK)) {
                    /* We have processed all incoming packets. */
//                    cout << "processed all incoming packets." << endl;
                } else {
                    perror("recv() error!");
                }
                break;
            }
            if (r > 0) {
                cout << "recv << " << buffer << endl;
            }
        }
        usleep(1);
    }
}

int main() {
    struct sockaddr_in     server_addr, local_addr;
    int optval = 1;
    socklen_t server_addr_len, local_addrLen;

    register_signals();

    running = true;

    memset(&server_addr, 0, sizeof(server_addr));

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
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

    if(make_socket_non_blocking(sockfd) < 0){
        perror( "Could make socket non blocking");
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

    std::thread thread_recv(recv_thread, sockfd,  (struct sockaddr *) &server_addr, &server_addr_len);
    std::thread thread_send(send_thread, sockfd);

    thread_send.detach();
    thread_recv.join();

    return 0;
}