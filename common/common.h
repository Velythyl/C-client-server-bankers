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
/*
 * On respecte toujours l'API de cmt_type; ERR_NEEDED et ERR_NEGA sont juste pour l'affichage en ligne de commande de
 * server_thread.c
 */
enum cmd_type {
    ERR_NEEDED = -2,
    ERR_NEGA = -1,
    BEGIN,
    CONF,
    INIT,
    REQ,
    ACK,
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

void read_socket(int sockfd, int *buf, size_t obj_sz);

void write_socket(int sockfd, void *buf, size_t obj_sz, int flags);

void *safeMalloc(size_t s);

void print_comm(int *arr, int size, bool print_enum, bool print_n);

int* read_compound(int socket_fd);

void write_compound(int socket, int head[2], int* message, bool print);

#endif
//BEGIN 1 7382479
//ACK 1 7382479

//ACK 0

