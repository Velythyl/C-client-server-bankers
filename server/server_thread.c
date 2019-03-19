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
int nb_registered_clients = 0;

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
int *ressources_max;   //r0, r1, ...
int *available;

pthread_mutex_t client_mutex;
typedef struct client client;   //chaque client aura sa representation client
struct client {
    unsigned int id;
    int* m_ressources;
    int* u_ressources;
    unsigned int closed;
};
client** clients;
int max_index_client;

void create_clients() {
    max_index_client = 2;

    clients = safeMalloc((max_index_client+1) * sizeof(client));

    for(int i=0; i<max_index_client; i++) {
        clients[i] = NULL;
    }
}

void resize_clients(int new_max_index) {
    if(new_max_index < max_index_client) return;

    client** old = clients;
    clients = safeMalloc((new_max_index+1) * sizeof(client));

    for(int i=0; i<max_index_client; i++) {
        clients[i] = old[i];
    }

    max_index_client = new_max_index;
    free(old);
}

void new_client(unsigned int id, int* ressources) {
    pthread_mutex_lock(&client_mutex);

    if(id > max_index_client) resize_clients(id);

    if(clients[id] == NULL) {
        client* cl = safeMalloc(sizeof(client));
        cl->m_ressources = ressources;
        cl->u_ressources = malloc(nb_ressources * sizeof(int));
        for(int i=0; i<nb_ressources; i++) {
            cl->u_ressources[i] = 0;
        }
        cl->id = id;
        cl->closed = 0;

        clients[id] = cl;

        fprintf(stdout, "Client %i: nb de m_ressources ", cl->id);
        print_comm(cl->m_ressources, nb_ressources, false, true);
    } else if(clients[id]->id != id) {
        exit(204);
    }

    nb_registered_clients++;
    pthread_mutex_unlock(&client_mutex);
}

void close_client(int index) {
    clients[index]->closed = 1;
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
    read_socket(new_socket, begin, sizeof(begin), TIMEOUT);   //attend 30 secondes: on laisse beaucoup de temps au client

    /*
     * Pourrait etre fait avec read_compound, mais plus efficace de le faire comme ceci: on doit aussi assigner
     * nb_ressources et ressources_max, donc il est plus simple de juste acceder aux sous-parties de read_compound
     */
    int conf1[2] = {-1, -1};
    read_socket(new_socket, conf1, sizeof(conf1), TIMEOUT);
    nb_ressources = conf1[1];

    ressources_max = safeMalloc(nb_ressources * sizeof(int));
    read_socket(new_socket, ressources_max, nb_ressources* sizeof(int), TIMEOUT);

    available = malloc(nb_ressources* sizeof(int));
    for(int i=0; i<nb_ressources; i++) {
        available[i] = ressources_max[i];   //au debut tout est avail
    }

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

    if (pthread_mutex_init(&client_mutex, NULL) != 0) {
        printf("\n mutex init has failed\n");
        exit(1);
    }
}

int bankers(int* request) {
    int* work = malloc(nb_ressources* sizeof(int));
    for(int i=0; i<nb_ressources; i++) {
        if(request[i]>available[i]) {
            free(work);
            return false;
        }
        work[i] = available[i];
    }

    int* finish = malloc((max_index_client+1)* sizeof(int));
    int nb_not_done=0;
    for(int i=0; i<max_index_client+1; i++) {
        if(clients[i]->closed == 0) {
            finish[nb_not_done] = false;
            nb_not_done++;
        }
    }
    finish[nb_not_done] = NULL;   //null terminate

    for(int i=0; i<nb_not_done; i++) {
        if(finish[i] == false) {
            int* needed = clients[i]->m_ressources;
            int all_lower = 1;

            for(int j=0; j<nb_ressources; j++) {
                if(needed[j] > work[j]) all_lower = 0;
            }

            if(all_lower) {
                for(int k=0; k<nb_ressources; k++) {
                    work[k] += request[k];
                    finish[i] = true;
                }
            }
        }
    }

    int safe = 1;
    for(int i=0; i<nb_not_done; i++) {
        if(finish[i] == false) safe = 0;
    }

    for(int i=0; i<nb_ressources; i++) {
        available[i] += work[i];
        if(available[i] < 0) available[i] = 0;
    }

    return safe;
}

void st_process_requests(server_thread *st, int socket_fd) {
    /*
     * Notes:
     *
     * premier truc recu sera un INI puis les REQ
     */

    int* cmd = read_compound(socket_fd);
    print_comm(cmd, cmd[1]+2, true, true);

    int response_head[2] = {-1, -1};
    int* response;
    int* ressources;
    switch(cmd[0]) {
        case END:
            //TODO
            break;
        case REQ:
            /*
            response_head[0] = WAIT;
            response_head[1] = 1;

            response = malloc(sizeof(int));
            response[0] = 2;*/

            /*
            ressources = malloc(nb_ressources * sizeof(int));

            for(int i=0; i<nb_ressources; i++) {
                ressources[i] = cmd[i+3];
            }

            if(bankers(ressources) == 1) {
                response_head[0] = ACK;
                response_head[1] = 0;
            } else {
                response_head[0] = WAIT;
                response_head[1] = 1;

                response = malloc(sizeof(int));
                response[0] = 5;
            }*/

            /*
            response_head[0] = ERR;
            response_head[1] = 4;
            response = malloc(4 * sizeof(int));
            response[0] = 'A';
            response[1] = 'L';
            response[2] = 'L';
            response[3] = 'O';*/

            /*
            response_head[0] = ACK;
            response_head[1] = 0;*/
            break;

            break;
        case INIT:
            ressources = malloc(nb_ressources * sizeof(int));

            for(int i=0; i<nb_ressources; i++) {
                ressources[i] = cmd[i+3];
            }
            new_client(cmd[2], ressources);

            response_head[0] = ACK;
            response_head[1] = 0;
            break;
         case CLO:
            break;
    }

    write_compound(socket_fd, response_head, response);

    free(response);

    close(socket_fd);

    // TODO: traiter la commande

}



void st_signal() {
    //TODO ecouter le socket pour savoir la quantite de ressources_max qu'on a



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

        int i=0;
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
