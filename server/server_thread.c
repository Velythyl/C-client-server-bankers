//#define _XOPEN_SOURCE 700   /* So as to allow use of `fdopen` and `getline`.  */

#include "common.h"
#include "server_thread.h"

#include <netinet/in.h>

#include <pthread.h>
#include <unistd.h>

#include <string.h>

enum {
    NUL = '\0'
};

enum {
    /* Configuration constants.  */
            max_wait_time = 30,
    server_backlog_size = 5
};

unsigned int server_socket_fd;  //id du socket que tous les threads utilisent

int port_number = -1;

// Nombre de client enregistré.
int nb_registered_clients;

// Variable du journal.
// Nombre de requêtes acceptées immédiatement (ACK envoyé en réponse à REQ).
unsigned int count_accepted = 0;

// Nombre de requêtes acceptées après un délai (ACK après REQ, mais retardé).
unsigned int count_wait = 0;

// Nombre de requête erronées (ERR envoyé en réponse à REQ).
unsigned int count_invalid = 0;

// Nombre de clients qui se sont terminés correctement
// (ACK envoyé en réponse à CLO).
unsigned int count_dispatched = 0;

// Nombre total de requête (REQ) traités.
unsigned int request_processed = 0;

// Nombre de clients ayant envoyé le message CLO.
unsigned int clients_ended = 0;

// TODO: Ajouter vos structures de données partagées, ici.

int nb_ressources = -1;
int *ressources;   //r0, r1, ...
int *available;

typedef struct client client;   //chaque client aura sa representation client
struct client {
    unsigned int id;
    int* ressources;
};
client** clients;
int max_index_client;

void create_clients() {
    max_index_client = 2;

    clients = safeMalloc(max_index_client * sizeof(client));

    for(int i=0; i<max_index_client; i++) {
        clients[i] = NULL;
    }
}

void resize_clients(int new_max_index) {
    if(new_max_index < max_index_client) return;

    client** old = clients;
    clients = safeMalloc(new_max_index * sizeof(client));

    for(int i=0; i<max_index_client; i++) {
        clients[i] = old[i];
    }

    max_index_client = new_max_index;
    free(old);
}

void new_client(int index, unsigned int id, int* ressources) {
    if(index > max_index_client) resize_clients(index+1);

    if(clients[index] == NULL) {
        client* cl = safeMalloc(sizeof(client));
        cl->ressources = ressources;
        cl->id = id;

        clients[index] = cl;
    } else if(clients[index]->id != id) {
        exit(204);
    }
}

void st_init() {
    // Initialise le nombre de clients connecté.
    nb_registered_clients = 0;

    st_open_socket();

    struct sockaddr_in addr;
    socklen_t socket_len = sizeof(addr);
    int new_socket = -1;
    int end_time = time(NULL) + max_wait_time;

    // Boucle jusqu'à ce que `accept` reçoive la première connection.
    while (new_socket < 0) {
        new_socket = accept(server_socket_fd, (struct sockaddr *) &addr, &socket_len);

        if (time(NULL) >= end_time) {
            break;
        }
    }

    int begin[3] = {-1, -1, -1};
    read_socket(new_socket, begin, sizeof(begin), 30000);   //attend 30 secondes: on laisse beaucoup de temps au client

    int conf1[2] = {-1, -1};
    read_socket(new_socket, conf1, sizeof(conf1), 30000);
    nb_ressources = conf1[1];

    ressources = safeMalloc(nb_ressources * sizeof(int));
    read_socket(new_socket, ressources, sizeof(ressources), 30000);

    int ok[3] = {ACK, 1, begin[2]};
    write(new_socket, ok, sizeof(ok));

    // Attend la connection d'un client et initialise les structures pour
    // l'algorithme du banquier.

    // END TODO
    close(new_socket);
    close(server_socket_fd);

    /*
     * nb initial de clients suppose etre deux (evidemment ici on pourrait mettre 5, mais ceci est plus robuste si
     * jamais on change le nb de petits clients
     */
    create_clients();
}

void st_process_requests(server_thread *st, int socket_fd) {
    /*
     * Notes:
     *
     * premier truc recu sera un INI puis les REQ
     */


    // TODO: Remplacer le contenu de cette fonction

    for (;;) {
        int head[2] = {-1, -1}; //prends la commande et son nb d'args
        read_socket(socket_fd, head, sizeof(head), max_wait_time * 100);

        int* cmd_receiver = safeMalloc(head[1] * sizeof(int));
        int len = read_socket(socket_fd, cmd_receiver, sizeof(cmd_receiver), max_wait_time * 100);

        if (len > 0) {
            if (len != sizeof(head[0]) && len != sizeof(head)) {
                printf("Thread %d received invalid command size=%d!\n", st->id, len);
                break;
            }
            printf("Thread %d received command=%d, nb_args=%d\n", st->id, head[0], head[1]);
            // dispatch of cmd void thunk(int sockfd, struct cmd_header* header);

            //ON TRAITE LES CMD ICI
            switch(head[0]) {
                case END:

                case INIT:
                    //TODO new_client!
                case REQ:

                case CLO:
                    break;
            }


        } else {
            if (len == 0) {
                fprintf(stderr, "Thread %d, connection timeout\n", st->id);
            }
            break;
        }
    }
    close(socket_fd);
}



void st_signal() {
    //TODO ecouter le socket pour savoir la quantite de ressources qu'on a



}

void *st_code(void *param) {
    server_thread *st = (server_thread *) param;

    struct sockaddr_in thread_addr;
    socklen_t socket_len = sizeof(thread_addr);
    int thread_socket_fd = -1;
    int end_time = time(NULL) + max_wait_time;

    // Boucle jusqu'à ce que `accept` reçoive la première connection.
    while (thread_socket_fd < 0) {
        thread_socket_fd = accept(server_socket_fd, (struct sockaddr *) &thread_addr, &socket_len);

        if (time(NULL) >= end_time) {
            break;
        }
    }

    // Boucle de traitement des requêtes.
    while (accepting_connections) {
        if (!nb_registered_clients && time(NULL) >= end_time) {
            fprintf(stderr, "Time out on thread %d.\n", st->id);
            pthread_exit(NULL);
        }
        if (thread_socket_fd > 0) {
            st_process_requests(st, thread_socket_fd);
            close(thread_socket_fd);
            end_time = time(NULL) + max_wait_time;
        }

        //prends les prochaines connections des clients qui n'ont pas encore ete traitees
        thread_socket_fd = accept(server_socket_fd, (struct sockaddr *) &thread_addr, &socket_len);
    }
    return NULL;
}


//
// Ouvre un socket pour le serveur.
//
void st_open_socket() {
    server_socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (server_socket_fd < 0) perror("ERROR opening socket");

    if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEPORT, &(int) {1}, sizeof(int)) < 0) {
        perror("setsockopt()");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_number);

    if (bind(server_socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        perror("ERROR on binding");

    listen(server_socket_fd, server_backlog_size);
}


//
// Affiche les données recueillies lors de l'exécution du
// serveur.
// La branche else ne doit PAS être modifiée.
//
void st_print_results(FILE *fd, bool verbose) {
    if (fd == NULL) fd = stdout;
    if (verbose) {
        fprintf(fd, "\n---- Résultat du serveur ----\n");
        fprintf(fd, "Requêtes acceptées: %d\n", count_accepted);
        fprintf(fd, "Requêtes : %d\n", count_wait);
        fprintf(fd, "Requêtes invalides: %d\n", count_invalid);
        fprintf(fd, "Clients : %d\n", count_dispatched);
        fprintf(fd, "Requêtes traitées: %d\n", request_processed);
    } else {
        fprintf(fd, "%d %d %d %d %d\n", count_accepted, count_wait,
                count_invalid, count_dispatched, request_processed);
    }
}
