#ifndef CLIENTTHREAD_H
#define CLIENTTHREAD_H

#include "common.h"

/* Port TCP sur lequel le serveur attend des connections.  */
extern int port_number;

/* Nombre de requêtes que chaque client doit envoyer.  */
extern int num_request_per_client;

/* Nombre de resources différentes.  */
extern int num_resources;

/* Quantité disponible pour chaque resource.  */
extern int *provisioned_resources;


typedef struct client_thread client_thread;
struct client_thread {
    unsigned int id;
    pthread_t pt_tid;
    pthread_attr_t pt_attr;
};


void ct_init(client_thread *);

void ct_create_and_start(client_thread *);

void ct_wait_server(int num_clients, client_thread* client_threads);

void ct_print_results(FILE *fd, bool verbose);

void init_client();

int c_open_socket();

#endif // CLIENTTHREAD_H
