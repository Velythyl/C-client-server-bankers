/* This `define` tells unistd to define usleep and random.  */
#define _XOPEN_SOURCE 500

#include <memory.h>
#include "client_thread.h"
#include "common.h"

#include <pthread.h>
#include <unistd.h>

int port_number = -1;
int num_request_per_client = -1;
int num_resources = -1;
int *provisioned_resources = NULL;

unsigned int client_socket_fd;

// Variable d'initialisation des threads clients.
unsigned int count = 0; //id des threads


// Variable du journal.
// Nombre de requête acceptée (ACK reçus en réponse à REQ)
unsigned int count_accepted = 0;

// Nombre de requête en attente (WAIT reçus en réponse à REQ)
unsigned int count_on_wait = 0;

// Nombre de requête refusée (REFUSE reçus en réponse à REQ)
unsigned int count_invalid = 0;

// Nombre de client qui se sont terminés correctement (ACC reçu en réponse à END)
unsigned int count_dispatched = 0;

// Nombre total de requêtes envoyées.
unsigned int request_sent = 0;

// retourne un nb de 0 a high-1
int random_bounded(int high) {
    return (int) (random() % high); //safe puisque high est un int, donc on est surs que random % high fitte dans un int
}


// Vous devez modifier cette fonction pour faire l'envoie des requêtes
// Les ressources demandées par la requête doivent être choisies aléatoirement
// (sans dépasser le maximum pour le client). Elles peuvent être positives
// ou négatives.
// Assurez-vous que la dernière requête d'un client libère toute les ressources
// qu'il a jusqu'alors accumulées.
void send_request(int client_id, int request_id, int socket_fd) {

    //pas tested...
    int* request = malloc((num_resources + 3) * sizeof(int));

    request[0] = REQ;
    request[1] = num_resources+1;
    request[2] = client_id;

    for(int i=0; i<num_resources; i++) {
        request[i+3] = random_bounded(provisioned_resources[i]);
    }

    fprintf(stdout, "Client %d is sending its %d request\n", client_id,
            request_id);

    // TP2 TODO:END

}


void *ct_code(void *param) {
    int socket_fd = -1;
    client_thread *ct = (client_thread *) param;

    // TP2 TODO
    // Vous devez ici faire l'initialisation des petits clients (`INI`).

    int* init = malloc((num_resources + 3)* sizeof(int));
    init[0] = INIT;
    init[1] = num_resources+1;
    init[2] = ct -> id;
    for(int i=0; i<num_resources+0; i++) {
        init[i+3] = random_bounded(provisioned_resources[i]+1);    //on veut de 0 a MAX RESSOURCE
    }

    int socket = c_open_socket();
    write(socket, init, sizeof(init));
    print_comm(init, 3, true, true);

    // TP2 TODO:END

    for (unsigned int request_id = 0; request_id < num_request_per_client;
         request_id++) {

        // TP2 TODO
        // Vous devez ici coder, conjointement avec le corps de send request,
        // le protocole d'envoi de requête.

        send_request(ct->id, request_id, socket_fd);

        // TP2 TODO:END

        /* Attendre un petit peu (0s-0.1s) pour simuler le calcul.  */
        usleep(random() % (100 * 1000));
        /* struct timespec delay;
         * delay.tv_nsec = random () % (100 * 1000000);
         * delay.tv_sec = 0;
         * nanosleep (&delay, NULL); */
    }

    pthread_exit(NULL);
}


//
// Vous devez changer le contenu de cette fonction afin de régler le
// problème de synchronisation de la terminaison.
// Le client doit attendre que le serveur termine le traitement de chacune
// de ses requêtes avant de terminer l'exécution.
//
void ct_wait_server() {

    // TP2 TODO

    sleep(4);

    // TP2 TODO:END

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
void ct_print_results(FILE *fd, bool verbose) {
    if (fd == NULL)
        fd = stdout;
    if (verbose) {
        fprintf(fd, "\n---- Résultat du client ----\n");
        fprintf(fd, "Requêtes acceptées: %d\n", count_accepted);
        fprintf(fd, "Requêtes : %d\n", count_on_wait);
        fprintf(fd, "Requêtes invalides: %d\n", count_invalid);
        fprintf(fd, "Clients : %d\n", count_dispatched);
        fprintf(fd, "Requêtes envoyées: %d\n", request_sent);
    } else {
        fprintf(fd, "%d %d %d %d %d\n", count_accepted, count_on_wait,
                count_invalid, count_dispatched, request_sent);
    }
}

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

    while(connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("connecting to server...\n");
    }

    return socket_fd;
}
