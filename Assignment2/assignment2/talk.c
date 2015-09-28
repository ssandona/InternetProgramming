#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#define BACKLOG 5
#define PORT 5553
#define NB_PROC 10

int socketfd;

void handler() {
    close(socketfd);
    exit(1);
}

int openSocket(struct sockaddr_in *server_addr) {
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    int enable = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("Error setting socket options");
    }

    if(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0) {
        perror("Error setting socket options");
    }

    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(PORT);
    server_addr->sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr *) server_addr,
             sizeof(struct sockaddr_in)) == -1) {
        perror("bind");
    }

    if (listen(fd, BACKLOG) == -1)
        perror("listen");

    return fd;
}


ssize_t writen(int fd, const void *vptr, size_t n) {
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if  ((nwritten = write(fd, ptr, nleft)) <= 0 ) {
            if (errno == EINTR)
                nwritten = 0; /* and call write() again */
            else
                return -1; /* error */
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

void sig_chld(int a) {
    while (waitpid(0, NULL, WNOHANG) > 0) {}
    close(socketfd);
    exit(1);
}

void treat_request_read() {
    char leggi[500];
    while(1) {
        if((recv(socketfd, leggi, 500, 0)) > 0) {
            printf("\nOther: %s", leggi);
        } else {
            handler();
            exit(1);
        }
    }

}

void treat_request_write() {
    char *msg;
    char enter[100] = "";
    size_t size = 0;

    while(1) {
        printf("\nMe: ");
        if(getline(&msg, &size, stdin) == -1) {
            printf("Couldn't read the input\n");
            handler();
            exit(1);
        }
        writen(socketfd, (void *)msg, strlen(msg) + 1);
    }
}

int main(int argc, char **argv) {
    if(argc == 2) {
        struct sockaddr_in remote;
        struct hostent *host;

        if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
        }

        host = gethostbyname(argv[1]);
        if (host == NULL) {
            fprintf(stderr, "Hostname \"%s\" could not be resolved\n", argv[1]);
            exit(-2);
        }
        char *addr[5];
        int i = 0;
        struct in_addr **q = (struct in_addr **)host->h_addr_list;
        while (*q != NULL) {
            addr[i] = malloc(16 * sizeof(char));
            strcpy(addr[i], inet_ntoa(**q));
            q++;
            ++i;
        }

        remote.sin_family = AF_INET;
        remote.sin_port = htons(PORT);
        remote.sin_addr.s_addr = inet_addr(addr[0]);
        printf("Addr: %s, PORT: %d\n", addr[0], PORT);
        if (connect(socketfd, (struct sockaddr *)&remote, sizeof(struct sockaddr_in)) == -1) {
            perror("connect");
            exit(1);
        }
    } else {
        int num;
        struct sockaddr_in server_addr, client_addr;;
        socketfd = openSocket(&server_addr);
        signal(SIGINT, handler);
        int yes = 1;
        /*ignore 'address already in use'*/
        if ( setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) {
            perror("setsockopt");
        }
        if ( setsockopt(socketfd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1 ) {
            perror("setsockopt");
        }
        int addrlen = sizeof(struct sockaddr_in);
        socketfd = accept(socketfd, (struct sockaddr *) &client_addr, &addrlen);
        printf("Request\n");
        if(socketfd < 0)
            exit(1);
    }


    signal(SIGINT, handler);


    if (fork() == 0) {
        treat_request_write();
    } else treat_request_read();

    return 0;
}
