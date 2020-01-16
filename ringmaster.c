
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include "potato.h"
#include <assert.h>
#include <sys/time.h>

int main(int argc, const char *argv[]) {
    if (argc != 4) {
        printf("argc wrong ! input invalid\n");
        return -1;
    }
    //assign
    const char* port_num = argv[1];//port num
    int player_num = atoi(argv[2]);
    int hops = atoi(argv[3]);
    //check
    printf("Potato Ringmaster\n");
    printf("Players = %d\n", player_num);
    printf("Hops = %d\n", hops);
    //getaddrinfo
    struct addrinfo host_info;
    struct addrinfo* host_pointer;
    const char* hostname = NULL;
    memset(&host_info, 0, sizeof(host_info));
    int ret = -1;
    //the general setting
    host_info.ai_flags = AI_PASSIVE;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_family = AF_UNSPEC;
    //getaddrinfo can transfer name to address, service to port
    ret = getaddrinfo(hostname, port_num, &host_info, &host_pointer);
    if (ret != 0) {
        printf("getaddrinfo error\n");
        return -1;
    }
    
    //socket()
    int sock_fd = socket(host_pointer->ai_family, host_pointer->ai_socktype, host_pointer->ai_protocol);
    if(sock_fd < 0) {
        printf("socket()");
        return -1;
    }
    //setsockopt() is used for allowing several client to connect the server at same time
    // the point is that after accept(), the program will create child process which is used to write and read, and the father process can still listen and accept
    int yes = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    //bind()
    int bind_fd = bind(sock_fd, host_pointer->ai_addr, host_pointer->ai_addrlen);
    if(bind_fd < 0) {
        printf("bind()");
        return -1;
    }
    //listen
    int listen_fd = listen(sock_fd, player_num);//what if the # player is too large
    if (listen_fd < 0) {
        printf("listen()");
        return -1;
    }
    
    //sockaddr_storage is used to uniformly represent sockaddr_in and sockaddr_in6.
    // so when usiing sockaddr, we need type change explictly
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_fd[player_num];
    struct Potato players[player_num];
    for (int i = 0; i < player_num; ++i) {
        //client's address
        client_fd[i] = accept(sock_fd, (struct sockaddr*)&socket_addr, &socket_addr_len);
        if (client_fd[i] < 0) {
            printf("accept()");
            return -1;
        }
        //get client's ip and port
        struct sockaddr_in * tmp = (struct sockaddr_in *) &socket_addr;
        const char * client_ip = inet_ntoa(tmp->sin_addr);
        int client_port = ntohs(tmp->sin_port);
        //asign to Potato struct
        players[i].client_fd = client_fd[i];
        players[i].ip = client_ip;
        players[i].port = client_port;
        
        //send information with client
        send(client_fd[i], &i, sizeof(int), 0);
        send(client_fd[i], &hops, sizeof(int), 0);
        send(client_fd[i], &player_num, sizeof(int), 0);
        /*it's ready of i player*/
        printf("player %d is ready to play\n", players[i].client_fd);
        
        if (i != 0) {
            //send previous neighbor
            send(client_fd[i], &players[i - 1], sizeof(struct Potato), 0);
            if (i == player_num - 1) {
                send(client_fd[0], &players[i], sizeof(struct Potato), 0);
            }
        }
    }
    /*send first potato*/
    struct route_potato _potato;
    _potato.hops = hops;
    _potato.player_num = player_num;
    srand((unsigned int) time(0));
    int random = rand()% player_num;
    printf("Ready to start the game, sending potato to %d\n",random);
    _potato.next_id = random;
    _potato.route[0] = _potato.next_id;
    send(client_fd[random], &_potato, sizeof(struct route_potato), 0);
    
    //listen the client
    int fmax = 0;
    fd_set fds;
    int exitwhile = 0;
    while(1) {
        
        FD_ZERO(&fds);//clean up the socket fd set
        for (size_t i = 0; i < player_num; ++i) {
            FD_SET(client_fd[i], &fds); // add the client_fd to the set
            if (fmax < client_fd[i]) fmax = client_fd[i];
        }
        
        int ret = select(fmax + 1,&fds, NULL, NULL, NULL);
        if (ret == -1) {
            printf("select()");
            return -1;
        }
        
        for (size_t i = 0; i < player_num; ++i) {
            if (FD_ISSET(client_fd[i], &fds)) {
               //so at the end of the game;
                long end = recv(client_fd[i], &_potato, sizeof(struct route_potato), MSG_WAITALL);
                if (end == -1) {
                    printf("error at the game end!!!");
                    return -1;
                }
                //print path
                for (int i = 0; i < hops; ++i) {
                    if (i == hops - 1) {
                        printf("%d\n", _potato.route[i]);
                    } else {
                        printf("%d,", _potato.route[i]);
                    }
                }
                    
                //stop the game
                for (int i = 0; i < player_num; ++i) {
                    struct route_potato stop;
                    stop.player_num = -1;
                    send(client_fd[i], &stop, sizeof(struct route_potato), 0);
                }
                exitwhile = 1;
                break;
            }
                
        }
        
        if (exitwhile) break;
    }
    
    freeaddrinfo(host_pointer);
    close(sock_fd);
    return 0;
    
}

