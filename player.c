

#include "player.h"

#include "potato.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <netdb.h>
#include <sys/time.h>

int main(int argc, char * argv[]) {
    if (argc != 3) {
        printf("argc wrong\n");
        return -1;
    }
    
    struct addrinfo host_info;
    struct addrinfo* host_info_pointer;
    const char *hostname = argv[1];//hostip
    const char *port = argv[2];
    
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    
    int status = getaddrinfo(hostname, port, &host_info, &host_info_pointer);
    if(status != 0) {
        printf("getaddrinfo()");
        return -1;
    }
    
    int socket_fd = socket(host_info_pointer->ai_family, host_info_pointer->ai_socktype, host_info_pointer->ai_protocol);
    if (socket_fd == -1) {
        printf("socket()");
        return -1;
    }
    
    
    
    
    
    
    int con_fd = connect(socket_fd, host_info_pointer->ai_addr, host_info_pointer->ai_addrlen);
    if (con_fd == - 1) {
        printf("connect()");
        return -1;
    }
    //********** connection with ringmaster completed ********//
    
    //********** from ringmaster : receive info********//
    //current player's own info
    int player_id = 0;
    int hops = 0;
    int player_num = 0;
    recv(socket_fd, &player_id, sizeof(int), 0);
    recv(socket_fd, &hops, sizeof(int), 0);
    recv(socket_fd, &player_num, sizeof(int), 0);
    printf("Connected as player %d out of %d players\n", player_id, player_num);
    
     /* construct player - player as server */
    struct addrinfo server_info;
    struct addrinfo* server_info_list;
    const char * server_hostname = NULL;
    memset(&server_info, 0, sizeof(server_info));
    server_info.ai_family = AF_UNSPEC;
    server_info.ai_socktype = SOCK_STREAM;
    server_info.ai_flags = AI_PASSIVE;
    int real_port = 10164 + player_id; //why 10164????
    char server_port[6];
    sprintf(server_port, "%d", real_port);
    int status_server = getaddrinfo(server_hostname,server_port, &server_info, &server_info_list);
    if (status_server != 0) {
        printf("getaddrinfo(), player-player server");
        return -1;
    }
    
    int server_fd = socket(server_info_list->ai_family,server_info_list->ai_socktype, server_info_list->ai_protocol);
    if (server_fd == -1) {
        printf("player-player server : socket()");
        return -1;
    }
    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(server_fd, server_info_list->ai_addr, server_info_list->ai_addrlen);
    if (status == -1) {
        printf("player-player server : bind()");
        return -1;
    }
    status = listen(server_fd, 10);
    if (status == -1) {
        printf("player-player server: listen()");
        return -1;
    }
    
    
    /*construct player-player as client */
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    struct Potato neighbor;
    struct addrinfo player_server_info;
    struct addrinfo* player_server_info_list;
    memset(&player_server_info, 0, sizeof(player_server_info));
    player_server_info.ai_family = AF_UNSPEC;
    player_server_info.ai_socktype = SOCK_STREAM;
    int client_fd;
    int neighbor_fd;
    //connect the prev neighbor
    //attention: player_id 0 and other player_id should saperate
    //that is ringmaster send player_id which is not 0 their own info then the info about their privous player!
    if (player_id == 0) {
        neighbor_fd = accept(server_fd, (struct sockaddr *)& socket_addr, &socket_addr_len);
        if (neighbor_fd == -1) {
            printf("accept()\n");
            return -1;
        }
        //socket_fd is the fd with ring master
        recv(socket_fd, &neighbor, sizeof(struct Potato), 0);
        //so player_server_info is the info of current player(client)'s previous player(server)
        int status_client = getaddrinfo(neighbor.ip, neighbor.port, &player_server_info, &player_server_info_list);
        if (status_client != 0) {
            printf("player_id 0, wrong\n");
            return -1;
        }
        client_fd = socket(player_server_info_list->ai_family,player_server_info_list->ai_socktype, player_server_info_list->ai_protocol);
        if (client_fd == -1) {
            printf("player_id 0 socket()\n");
            return -1;
        }
        status_client = connect(client_fd, player_server_info_list->ai_addr, player_server_info_list->ai_addrlen);
        if (status_client == -1) {
            printf("player_id 0 connect()");
        }
    } else {
         //socket_fd is the fd with ring master, from ring maset get the info of previous player
        recv(socket_fd,&neighbor, sizeof(struct Potato), 0);
        int status_client = getaddrinfo(neighbor.ip, neighbor.port, &player_server_info, &player_server_info_list);
        if (status_client != 0) {
            printf("player_id, wrong\n");
            return -1;
        }
        client_fd = socket(player_server_info_list->ai_family,player_server_info_list->ai_socktype, player_server_info_list->ai_protocol);
        if (client_fd == -1) {
            printf("player_id socket()");
            return -1;
        }
        status_client = connect(client_fd, player_server_info_list->ai_addr, player_server_info_list->ai_addrlen);
        if (status_client == -1) {
            printf("player_id connect()");
        }
        
        if (player_id !=  player_num - 1) {
            neighbor_fd = accept(server_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
            if (neighbor_fd == -1) {
                printf("can not accept with neigbor\n");
                return -1;
            }
        } else {
            int ok = 1;
            //tell ringmaster every is alright
            send(socket_fd, &ok, sizeof(int), 0);
            neighbor_fd = accept(server_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
            if (neighbor_fd == -1) {
                printf("can not accept with neigbor\n");
                return -1;
            }
        }
    }
    //********** finish all connection!!! ********//
    
    fd_set three_fds;
    int next_player = player_id;
    srand( (unsigned int) time(NULL) + next_player);
    int judge;
    int max_fd = 0;
    
    while(1) {
        int fd[3] = {socket_fd, neighbor_fd, client_fd};//ringmaster, server itself, client itself with other player
        FD_ZERO(&three_fds);
        for (int i = 0; i < 3; ++i) {
            FD_SET(fd[i], &three_fds);
            if (max_fd < fd[i]) {
                max_fd = fd[i];
            }
        }
        
        if (select(max_fd + 1, &three_fds, NULL, NULL, NULL) == -1) {
            printf("select()\n");
        }
        
        /*receive potato first time*/
        if (FD_ISSET(socket_fd, &three_fds)) {
            
            struct route_potato potato;
            long cnt = recv(socket_fd, &potato, sizeof(struct route_potato), MSG_WAITALL);
            potato.next = 1;
            if (cnt <= 0) {
                printf("wrong recv first potato\n");
                return -1;
            }
            if (potato.player_num == -1) {
                close(server_fd);
                break;
            }
            judge = rand() % 2;
            //give potato to its left (previous potato)
            if (judge == 1) {
                if(potato.next_id == 0) {
                    potato.next_id = potato.player_num - 1;
                } else {
                    potato.next_id = player_id - 1;
                }
                
                printf("Ready to start the game, sending potato to %d\n", potato.next_id);
                potato.route[potato.next] = potato.next_id;
                potato.next++;
                send(client_fd, &potato, sizeof(struct route_potato), 0);
            } else {
                //send potato to its next player
                potato.next_id = player_id + 1 % player_num;
                potato.route[potato.next] = potato.next_id;
                potato.next++;
                send(neighbor_fd, &potato, sizeof(struct route_potato), 0);
            }
        }
        //recieve the potato from the player next to it. (it as server)
        if (FD_ISSET(neighbor_fd, &three_fds)) {
            judge = rand() % 2;
            struct route_potato potato;
            recv(neighbor_fd, &potato, sizeof(struct route_potato),MSG_WAITALL);
            if (potato.hops > 0) {
                potato.hops--;
                //give potato to its left (previous potato)
                if (judge == 1) {
                    if(potato.next_id == 0) {
                        potato.next_id = potato.player_num - 1;
                    } else {
                        potato.next_id = player_id - 1;
                    }
                    printf("Ready to start the game, sending potato to %d\n", potato.next_id);
                    potato.route[potato.next] = potato.next_id;
                    potato.next++;
                    send(client_fd, &potato, sizeof(struct route_potato), 0);
                } else {//send to next player
                    potato.next_id = player_id + 1 % player_num;
                    printf("Ready to start the game, sending potato to %d\n", potato.next_id);
                    potato.route[potato.next] = potato.next_id;
                    potato.next++;
                    send(neighbor_fd, &potato, sizeof(struct route_potato), 0);
                }
            } else {
                //game over
                printf("I'm it\n");
                send(socket_fd,&potato,sizeof(struct route_potato),0);
            }
        }
        //recive potato from its previous player
        if (FD_ISSET(client_fd, &three_fds)) {
            judge = rand() % 2;
            struct route_potato potato;
            recv(client_fd, &potato, sizeof(struct route_potato),MSG_WAITALL);
            //same code...
            if (potato.hops > 0) {
                potato.hops--;
                //give potato to its left (previous potato)
                if (judge == 1) {
                    if(potato.next_id == 0) {
                        potato.next_id = potato.player_num - 1;
                    } else {
                        potato.next_id = player_id - 1;
                    }
                    printf("Ready to start the game, sending potato to %d\n", potato.next_id);
                    potato.route[potato.next] = potato.next_id;
                    potato.next++;
                    send(client_fd, &potato, sizeof(struct route_potato), 0);
                } else {//send to next player
                    potato.next_id = player_id + 1 % player_num;
                    printf("Ready to start the game, sending potato to %d\n", potato.next_id);
                    potato.route[potato.next] = potato.next_id;
                    potato.next++;
                    send(neighbor_fd, &potato, sizeof(struct route_potato), 0);
                }
            } else {
                //game over
                printf("I'm it\n");
                send(socket_fd,&potato,sizeof(struct route_potato),0);
            }
        }
        
    }
    
    freeaddrinfo(host_info_pointer);//ringmaster
    freeaddrinfo(player_server_info_list);// player to player
    freeaddrinfo(server_info_list);// it as server
    close(socket_fd);
    close(client_fd);
    close(neighbor_fd);
    return 0;
}
