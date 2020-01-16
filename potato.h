
#include<stdio.h>
struct Potato {
    int client_fd;
    int port;
    const char* ip;
};

struct route_potato {
    int route[512];
    int player_num;
    int hops;
    int next;
    unsigned int next_id;
};;
