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
unsigned int count_wait = 0;    //TODO ????????????

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
int *available;

pthread_mutex_t client_mutex;
typedef struct client client;   //chaque client aura sa representation client
struct client {
    unsigned int id;
    int* m_ressources;
    int* u_ressources;
    unsigned int nb_of_wait;
};
client** clients;
int max_index_client;

void create_clients() {
    max_index_client = 2;

    clients = safeMalloc((max_index_client+1) * sizeof(client));

    for(int i=0; i<max_index_client; i++) clients[i] = NULL;
}

void resize_clients(int new_max_index) {
    if(new_max_index < max_index_client) return;

    client** old = clients;
    clients = safeMalloc((new_max_index+1) * sizeof(client*));

    for(int i=0; i<new_max_index+1; i++) {
        clients[i] = NULL;
    }

    for(int i=0; i<max_index_client+1; i++) {
        clients[i] = old[i];
    }

    max_index_client = new_max_index;
    free(old);  //free pas les clients de old car ils sont maintenant dans clients
}

void new_client(unsigned int id, int* ressources) {
    pthread_mutex_lock(&client_mutex);

    resize_clients(id); //s'assure que ID fitte dans clients[ID]

    if(clients[id] == NULL) {
        client* cl = safeMalloc(sizeof(client));
        cl->m_ressources = ressources;

        cl->u_ressources = safeMalloc(nb_ressources * sizeof(int));
        for(int i=0; i<nb_ressources; i++) cl->u_ressources[i] = 0;

        cl->id = id;
        cl->nb_of_wait = 0;

        clients[id] = cl;

        fprintf(stdout, "Client %i: nb de m_ressources ", cl->id);
        print_comm(cl->m_ressources, nb_ressources, false, true);
    } else if(clients[id]->id != id) {
        exit(204);
    }

    __atomic_add_fetch(&nb_registered_clients, 1, __ATOMIC_SEQ_CST);
    pthread_mutex_unlock(&client_mutex);
}

void close_client(int index) {
    clients[index]->id = NULL;  //on ferme le client en lui donnant NULL
    free(clients[index]->u_ressources);
    free(clients[index]->m_ressources);
}

void st_init() {
    nb_registered_clients = 0;  // Initialise le nombre de clients connecté.

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

    available = safeMalloc(nb_ressources * sizeof(int));
    read_socket(new_socket, available, nb_ressources* sizeof(int), TIMEOUT);

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
    if (pthread_mutex_init(&bankers_mutex, NULL) != 0) {
        printf("\n mutex init has failed\n");
        exit(1);
    }
}

/*
 * Fait l'algo du banquier sur une situation hypothetique. Available et les u_ressources des clients refletent cette
 * situation, et work et finish sont des tableaux de la bonne grandeur deja malloc.
 *
 * Puisqu'on appelle toujours bankers de call_bankers, on peut y gerer les free de work et finish: on s'evite de devoir
 * les free avant chaque cas qui return
 */
int bankers(int *work, int *finish) {
    //etape 1: init
    for(int i=0; i<nb_ressources; i++) {
        work[i] = available[i];                //Work = Available et Available[j] -= Requestedi[j] (for all j)
    }

    //Finish[ i ] = false for i = 0, 1, …, n - 1
    for(int i=0; i<max_index_client+1; i++) {
        if(clients[i] == NULL) finish[i] = true;            //clients non-init sont safes
        else if(clients[i]->id == NULL) finish[i] = true;   //clients fermes sont safes
        else finish[i] = false;                             //autres sont potentiellements pas safes
    }


    //etape 2
    while(true) {
        int found_i = -1;                               //le i qu'on trouve est initialement -1

        for(int i=0; i<max_index_client+1; i++) {
            if(finish[i] == false) {                    //(a) Finish[ i ] == false

                bool one_needed_gt_work = false;        //assume aucun needed[i] > work[i]
                for(int j=0; j<nb_ressources; j++) {
                    int needed_j = clients[i]->m_ressources[j] - clients[i]->u_ressources[j];
                    if(needed_j > work[j]) {            //si ce qu'on a encore besoin apres avoir hypothetiquement accepte la REQ > work
                        one_needed_gt_work = true;
                        break;
                    }
                }

                if(one_needed_gt_work == false) {       //(b) Needed[i,j] ≤ Work[ j ] (for all j)
                    found_i = i;                        // on a trouve un i!
                    break;
                }
            }
        }

        if(found_i == -1) break;    //si on a pas trouve de i: on passe a l'etape 4
        //etape 3
        else {
            for(int j=0; j<nb_ressources; j++) {
                work[j] += clients[found_i]->u_ressources[j];    //Work[ j ] += Allocated[ i, j] (for all j)
            }
            finish[found_i] = true;
        }
    };

    //etape 4
    bool safe = true;
    for(int i=0; i<max_index_client+1; i++) {
        if(finish[i] == false) {
            safe = false;
            break;
        }
    }

    if(safe == false) return WAIT;
    else return ACK;
}

/*
 * Permet d'initialiser les structures pour le banquier et de les free elegamment.
 *
 * De plus, separe la logique de requete/index de celle de work/finish
 */
int call_bankers(int* request, int index) {
    client* cl = clients[index];
    if(cl == NULL) return ERR;                                                      //si essaie d'allouer a un client pas init
    for(int i=0; i<nb_ressources; i++) {
        if(request[i] > (cl->m_ressources[i] - cl->u_ressources[i])) return ERR_NEEDED;    //Si Requestedi[ j ] ≤ Needed[ i,j ] for any j erreur!
        if((request[i]<0) && ((cl->u_ressources[i] + request[i])<0)) return ERR_NEGA;    //si free plus  qu'on a
    }

    for(int i=0; i<nb_ressources; i++) {
        cl->u_ressources[i] += request[i];
        available[i] -= request[i];
    }

    fprintf(stdout, "Analyzing REQ ");
    print_comm(request, nb_ressources, false, false);

    fprintf(stdout, "\t//Res A: ");
    print_comm(available, nb_ressources, false, false);

    fprintf(stdout, "\t//Client %d M: ", index);
    print_comm(cl->m_ressources, nb_ressources, false, false);

    fprintf(stdout, "\t//Client %d U: ", index);
    print_comm(cl->u_ressources, nb_ressources, false, false);

    int* work = safeMalloc(nb_ressources*sizeof(int));
    int* finish = safeMalloc((max_index_client+1)*sizeof(int));

    int code = bankers(work, finish);
    free(work), free(finish);

    if(code != ACK) {
        for(int i=0; i<nb_ressources; i++) {
            cl->u_ressources[i] -= request[i];
            available[i] += request[i];
        }
    }

    return code;
}

/*
 * Permet de verouiller les strucutures pour bankers, et ensuite d'imprimer le code de retour de l'algo
 */
pthread_mutex_t bankers_mutex;  // le mutex de l'algo du banquier
int lock_bankers(int *request, int index) {
    pthread_mutex_lock(&bankers_mutex); //lock car on depend de available
    pthread_mutex_lock(&client_mutex);

    int code = call_bankers(request, index);
    fprintf(stdout, "\t//");
    print_comm(&code, 1, true, true);

    pthread_mutex_unlock(&bankers_mutex);
    pthread_mutex_unlock(&client_mutex);

    return code;
}

int* error_builder(const char* message, int size) {
    int* err = safeMalloc(size* sizeof(int));

    for(int i=0; i<size; i++) {
        err[i] = message[i];
    }

    return err;
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
    int* response = NULL;
    int* ressources;
    bool ok;
    switch(cmd[0]) {
        case REQ:
            __atomic_add_fetch(&request_processed, 1, __ATOMIC_SEQ_CST);

            ressources = safeMalloc(nb_ressources * sizeof(int));

            for(int i=0; i<nb_ressources; i++) ressources[i] = cmd[i+3];


            switch (lock_bankers(ressources, cmd[2])) {
                case ERR_NEEDED:
                    response_head[0] = ERR;
                    response_head[1] = 28;
                    response = error_builder("REQ+USED greater than NEEDED", 28);
                    break;
                case ERR_NEGA:
                    response_head[0] = ERR;
                    response_head[1] = 26;
                    response = error_builder("REQ freeing more than USED", 26);
                    break;
                case ERR:
                    response_head[0] = ERR;
                    response_head[1] = 7;
                    response = error_builder("CLI NUL", 7);
                case ACK:
                    response_head[0] = ACK;
                    response_head[1] = 0;

                    __atomic_add_fetch(&count_accepted, 1, __ATOMIC_SEQ_CST);
                    break;
                case WAIT:
                    response_head[0] = WAIT;
                    response_head[1] = 1;

                    response = safeMalloc(sizeof(int));
                    response[0] = 1;

                    __atomic_add_fetch(&count_wait, 1, __ATOMIC_SEQ_CST);
                    break;
            }

            if(response_head[0] == ERR) __atomic_add_fetch(&count_invalid, 1, __ATOMIC_SEQ_CST);

            free(ressources);
            break;
        case INIT:
            ressources = safeMalloc(nb_ressources * sizeof(int));

            for(int i=0; i<nb_ressources; i++) {
                ressources[i] = cmd[i+3];
            }
            new_client(cmd[2], ressources);

            response_head[0] = ACK;
            response_head[1] = 0;
            break;
        case END:
            while(true) {
                int nb_done = 0;
                for (int i = 0; i < max_index_client; i++) {
                    /*
                     * normalement, on devrait avoir un mutex ici pour regarder la valeur de id.
                     *
                     * Cependant, on peut voir cette boucle while(true) comme un grand spinlock: comme les id ne se font changer
                     * que dans leur propres threads, et qu'ici on ne fait que regarder leur valeur, pas besoin de mutex.
                     * En effet, si le id n'est pas NULL lorsqu'on le regarde, ce n'est pas grave: on sleep(5) et on reessaie au
                     * prochain tour de boucle
                     */
                    if (clients[i]->id == NULL) nb_done++;
                    if(clients[i] == NULL) {
                        //TODO envoyer une erreur
                        exit(201);
                    }
                }
                if(nb_done == max_index_client) break;
                sleep(2);
            }

            //pret a fermer
            for(int i=0; i<max_index_client+1; i++) {
                free(clients[i]);   //le free de leurs int* a ete fait dans close_client
            }
            free(clients);

            response_head[0] = ACK;
            response_head[1] = 0;

            accepting_connections = false;  //en mettant ceci a false, les threads vont sortir de la loop dans st_code
            break;
        case CLO:
            __atomic_add_fetch(&clients_ended, 1, __ATOMIC_SEQ_CST);

            ok = true;
            for(int i=0; i<nb_ressources; i++) {
                if(clients[cmd[2]]->u_ressources[i] != 0) {
                    ok = false;
                    break;
                }
            }

            if(!ok) {
                response_head[0] = ERR;
                response_head[1] = 9;
                response = error_builder("CLO error", 9);
            } else {
                close_client(cmd[2]);

                response_head[0] = ACK;
                response_head[1] = 0;

                __atomic_add_fetch(&count_dispatched, 1, __ATOMIC_SEQ_CST);
            }

            break;
        case BEGIN:
        case CONF:
            response_head[0] = ERR;
            response_head[1] = 26;

            response = error_builder("BEGIN/CONF not valid here", 26);
            break;
    }

    write_compound(socket_fd, response_head, response);

    free(cmd);
    if(response != NULL) free(response);
    //pas de free(ressources) car on place son pointeur dans new_client

    close(socket_fd);
}



void st_signal() {
    //TODO ecouter le socket pour savoir la quantite de ressources_max qu'on a



}

void* st_code(void *param) {
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
    /*
     * Note: pas besoin de mutex pour accepting_connections: le seul endroit ou on le change arrive lorsqu'on sait qu'on
     * acceptera plus de connections. En fait, on traite le cas que traiterait un mutex dans process_request: si on
     * detecte qu'on pourrait encore avoir besoin de threads (aka si on avait besoin d'un mutex) on envoie une erreur
     * au client et on fait planter le serveur...
     */
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

    pthread_exit(0);    //une fois qu'on accepte plus de connections, on exit
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
