/* This `define` tells unistd to define usleep and random.  */
#define _XOPEN_SOURCE 500

#include <memory.h>
#include <stdatomic.h>
#include "client_thread.h"

int port_number = -1;
int num_request_per_client = -1;
int num_resources = -1;
int *provisioned_resources = NULL;

// Variable d'initialisation des threads clients.
unsigned int count = 0; //id des threads


// Variable du journal.
// Nombre de requête acceptée (ACK reçus en réponse à REQ)
unsigned int count_accepted = 0;

// Nombre de requête en attente (WAIT reçus en réponse à REQ)
unsigned int count_on_wait = 0;

// Nombre de requête refusée (REFUSE reçus en réponse à REQ)
unsigned int count_invalid = 0; //NOTE: on assume que c'est les ERR recus a REQ

// Nombre de client qui se sont terminés correctement (ACC reçu en réponse à END)
unsigned int count_dispatched = 0;  //NOTE: on assume que c'est en reponse a CLO*

// Nombre total de requêtes envoyées.
unsigned int request_sent = 0;  //NOTE: on assume que c'est le nombre de REQ, pas le nombre de REQ uniques

pthread_mutex_t random_mutex;
/**
 * Retourne un nb de 0 a high-1
 *
 * @param high  la borne superieure excluse du nb
 * @return      le nb
 */
int random_bounded(int high) {
    if(high == 0) return 0; //error handling simple

    /*
     * On doit proteger l'appel a random car random n'est pas thread safe: il se re-seed avec la valeur de retour pour
     * le prochain appel. Donc, si on ne le protegeait pas, on aurait souvent les memes valeurs dans differentes REQ si
     * plusieurs clients font l'appel a random() simultanement.
     *
     * https://linux.die.net/man/3/random
     */
    pthread_mutex_lock(&random_mutex);
    long r = random();
    pthread_mutex_unlock(&random_mutex);

    if(r<0) r= -r;          //abs du random

    return (int) (r % high); //safe puisque high est un int, donc on est surs que random % high fitte dans un int
}

void init_client() {
    if (pthread_mutex_init(&random_mutex, NULL) != 0) {
        printf("\n mutex init has failed\n");
        exit(1);
    }
}

/**
 * Envoie une requete REQ dont la tete est head et le corps est request.
 *
 * Traite le code de reponse a cette requete, puis le return
 *
 * @param ct        Le client thread qui envoie le REQ
 * @param head      La tete de REQ
 * @param request   Le corps de REQ
 * @return          Le code de reponse
 */
int send_request(client_thread* ct, int* head, int* request, int* used_ressources) {
    __atomic_add_fetch(&request_sent, 1, __ATOMIC_SEQ_CST);

    int socket = c_open_socket();

    write_compound(socket, head, request, true);  //lance la REQ

    int* response = read_compound(socket);  //recoit la reponse

    close(socket);

    int code = response[0];

    switch(code) {
        case WAIT:
            __atomic_add_fetch(&count_on_wait, 1, __ATOMIC_SEQ_CST);
            sleep((unsigned int) response[2]);  //attend le nb de sec dans WAIT
            break;

        case ACK:
            for(int i=0; i<num_resources; i++) {
                used_ressources[i] += request[i+1]; //REQ acceptee donc on update ct->used_ressources
            }
            __atomic_add_fetch(&count_accepted, 1, __ATOMIC_SEQ_CST);
            break;

        case ERR:
            __atomic_add_fetch(&count_invalid, 1, __ATOMIC_SEQ_CST);
            break;

        default:
            fprintf(stderr, "WRONG COMMAND");
            exit(21);
    }

    free(response);
    return code;
}

/**
 * Fin du client ct
 *
 * 1. Envoie CLO
 * 2. Recoit la reponse
 * 3. Se ferme en mettant ct->id a NULL
 *
 * @param ct    le client qui se ferme
 */
void ct_end(client_thread* ct) {
    int socket = c_open_socket();

    int head[2] = {CLO, 1};

    write_compound(socket, head, &ct->id, true);

    int* response = read_compound(socket);

    close(socket);

    if(response[0] == ERR) {    //
        exit(204);
    } else if(response[0] == ACK) {
        __atomic_add_fetch(&count_dispatched, 1, __ATOMIC_SEQ_CST);
    }

    free(response);
    ct->id = NULL;
}

/**
 * Le code principal des clients. Lance leur INIT, puis genere des REQ qu'on envoie avec send_request
 *
 * @param param le client
 * @return
 */
void *ct_code(void *param) {
    int socket = c_open_socket();
    client_thread *ct = (client_thread *) param;

    int init[2] = {INIT, num_resources+1};
    write_socket(socket, init, sizeof(init), 0);
    print_comm(init, 2, true, false);

    int* max_ressources = safeMalloc(num_resources* sizeof(int));    //le max de ressources du client
    int* used_ressources = safeMalloc(num_resources*sizeof(int));    //les ressources utilisees du client

    //Genere la commande de depart
    int* init_cmd = safeMalloc((num_resources+1)* sizeof(int));
    init_cmd[0] = ct->id;
    for (int i = 0; i < num_resources; i++) {
        init_cmd[i + 1] = random_bounded(provisioned_resources[i] + 1);    //on veut de 0 a MAX RESSOURCE

        max_ressources[i] = init_cmd[i+1];  //max est le random de provisionned (donc init[i])
        used_ressources[i] = 0;             //used est tout a 0
    }

    //lance le init
    write_socket(socket, init_cmd, (num_resources+1)* sizeof(int), 0);
    print_comm(init_cmd, num_resources+1, false, true);

    free(init_cmd);

    int* response = read_compound(socket);
    if(response[0] != ACK) {
        fprintf(stderr, "INIT ERR");
        exit(490);
    }

    free(response);
    close(socket);

    for (unsigned int request_id = 0; request_id < num_request_per_client; request_id++) {

        int *request = safeMalloc((num_resources + 1) * sizeof(int));
        request[0] = ct->id;
        for (int i = 0; i < num_resources; i++) {

            if(request_id == num_request_per_client-1) {        //si derniere REQ:
                request[i + 1 ] = -(used_ressources[i]);    //libere tout ce qu'on avait

            } else {                                            //sinon
                //si positif: de 0 a (max de ressource i +1)-1
                if(random_bounded(2)) request[i + 1] = random_bounded(max_ressources[i]+1);
                //sinon: de 0 a (used i +1)-1
                else request[i + 1] = -random_bounded(used_ressources[i]+1);
            }
        }

        int head[2] = {REQ, num_resources+1};
        fprintf(stdout, "Client %d is preparing its %d request\t", ct->id, request_id);

        while(send_request(ct, head, request, used_ressources) == WAIT); //Si on recoit un wait, on relance la REQ

        free(request);

        /* Attendre un petit peu (0s-0.1s) pour simuler le calcul.  */
        usleep(random() % (100 * 1000));
    }

    //ici, on a fait toutes nos request et on en est a fermer les clients
    ct_end(ct);

    free(max_ressources), free(used_ressources);

    pthread_exit(0);
}


/**
 * Dit au serveur de se fermer et attend sa reponse
 *
 * @param num_clients
 * @param client_threads
 */
void ct_wait_server(int num_clients, client_thread* client_threads) {

    //busy wait en attendant que tous les clients soient CLO
    while(true) {
        sleep(2);
        int nb_done = 0;
        for (int i = 0; i < num_clients; i++) {
            /*
             * normalement, on devrait avoir un mutex ici pour regarder la valeur de id.
             *
             * Cependant, on peut voir cette boucle while(true) comme un grand spinlock: comme les id ne se font changer
             * que dans leur propres threads, et qu'ici on ne fait que regarder leur valeur, pas besoin de mutex.
             * En effet, si le id n'est pas NULL lorsqu'on le regarde, ce n'est pas grave: on sleep(5) et on reessaie au
             * prochain tour de boucle
             */
            if (client_threads[i].id == NULL) nb_done++;
        }

        if(nb_done == num_clients) break;
    }

    int socket = c_open_socket();
    int end[2] = {END, 0};

    write_socket(socket, end, 2* sizeof(int), 0);
    print_comm(end, 2, true, true);

    int* response = read_compound(socket);  //ceci fait attendre le client
    if(response[0] != ACK) {
        fprintf(stderr, "END error");
        exit(94);
    }
    free(response);

    pthread_mutex_destroy(&random_mutex);
}

void ct_create_and_start(client_thread *ct) {
    ct->id = count++;   //provient de ct_init()

    pthread_attr_init(&(ct->pt_attr));
    pthread_attr_setdetachstate(&(ct->pt_attr), PTHREAD_CREATE_DETACHED);
    pthread_create(&(ct->pt_tid), &(ct->pt_attr), &ct_code, ct);
}

//
// Affiche les données recueillies lors de l'exécution du
// serveur.
// La branche else ne doit PAS être modifiée.
//

//NOTE: on a ajoute fflush car parfois ca n'imprimait pas
void ct_print_results(FILE *fd, bool verbose) {
    if (fd == NULL) fd = stdout;

    if (verbose) {
        fprintf(fd, "\n---- Résultat du client ----\n");
        fflush(fd);
        fprintf(fd, "Requêtes acceptées: %d\n", count_accepted);
        fflush(fd);
        fprintf(fd, "Requêtes : %d\n", count_on_wait);
        fflush(fd);
        fprintf(fd, "Requêtes invalides: %d\n", count_invalid);
        fflush(fd);
        fprintf(fd, "Clients : %d\n", count_dispatched);
        fflush(fd);
        fprintf(fd, "Requêtes envoyées: %d\n", request_sent);
        fflush(fd);
    } else {
        fprintf(fd, "%d %d %d %d %d\n", count_accepted, count_on_wait,
                count_invalid, count_dispatched, request_sent);
        fflush(fd);
    }
}

/**
 * Ouvre un socket et le connecte au serveur.
 *
 * @return  le socket
 */
int c_open_socket() {
    int socket_fd = -1;
    socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (socket_fd < 0) perror("ERROR opening socket");

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    //https://stackoverflow.com/questions/16508685/understanding-inaddr-any-for-socket-programming
    //https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(port_number);

    printf("connecting to server...\t");
    while (connect(socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0);

    return socket_fd;
}
