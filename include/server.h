#ifndef SERVER_H
#define SERVER_H
#include "location.h"

struct Server
{
    int id;
    char country[100];
    char city[100];
    char provider[100];
    char host[256];
};

int find_server(int id, struct Server *server);

int find_best_server(struct Location *location, struct Server *server);

#endif