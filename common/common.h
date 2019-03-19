#ifndef TP2_COMMON_H
#define TP2_COMMON_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

//POSIX library for threads
#include <pthread.h>
#include <unistd.h>

#include <sys/types.h>
#include <poll.h>
#include <sys/socket.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TIMEOUT 60000

enum cmd_type {
    BEGIN,
    CONF,
    INIT,
    REQ,
    ACK,// Mars Attack
    WAIT,
    END,
    CLO,
    ERR,
    NB_COMMANDS
};

typedef struct cmd_header_t {
    enum cmd_type cmd;
    int nb_args;
} cmd_header_t;

ssize_t read_socket(int sockfd, void *buf, size_t obj_sz, int timeout);

void *safeMalloc(size_t s);

void print_comm(int *arr, int size, bool print_enum, bool print_n);

int* read_compound(int socket_fd);

void write_compound(int socket, int head[2], int* message);

#endif
//BEGIN 1 7382479
//ACK 1 7382479

//ACK 0

