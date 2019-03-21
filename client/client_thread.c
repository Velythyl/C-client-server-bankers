/* This `define` tells unistd to define usleep and random.  */
#define _XOPEN_SOURCE 500

#include <memory.h>
#include "client_thread.h"

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
// Les ressources_max demandées par la requête doivent être choisies aléatoirement
// (sans dépasser le maximum pour le client). Elles peuvent être positives
// ou négatives.
// Assurez-vous que la dernière requête d'un client libère toute les ressources_max
// qu'il a jusqu'alors accumulées.
void send_request(client_thread* ct, int* head, int* request) {
    int socket = c_open_socket();

    write_compound(socket, head, request);
    print_comm(head, 2, true, false);
    print_comm(request, num_resources+1, false, true);

    int* response = read_compound(socket);
    print_comm(response, response[1]+2, true, true);

    switch(response[0]) {
        case WAIT:
            close(socket);
            sleep((unsigned int) response[2]);
            send_request(ct, head, request);
            return;
        case ACK:
            for(int i=0; i<num_resources; i++) {
                ct->used_ressources[i] += request[i+1];
            }
            break;
        case ERR:
            break;
    }

    close(socket);
}

void ct_end(client_thread* ct) {
    int socket = c_open_socket();

    int head[2] = {CLO, 1};

    write_compound(socket, head, &ct->id);
    print_comm(head, 2, true, false);
    print_comm(&ct->id, 1, false, true);

    close(socket);

    ct->id = NULL;
}


void *ct_code(void *param) {
    int socket = c_open_socket();
    client_thread *ct = (client_thread *) param;

    int init[2] = {INIT, num_resources+1};
    write(socket, init, sizeof(init));
    print_comm(init, 2, true, false);

    int* init_cmd = malloc((num_resources+1)* sizeof(int));
    init_cmd[0] = ct->id;
    for (int i = 0; i < num_resources; i++) {
        init_cmd[i + 1] = random_bounded(provisioned_resources[i] + 1);    //on veut de 0 a MAX RESSOURCE

        ct->max_ressources[i] = init_cmd[i+1];  //max est le random de provisionned
        ct->used_ressources[i] = 0;             //used est tout a 0
    }

    write(socket, init_cmd, (num_resources+1)* sizeof(int));
    print_comm(init_cmd, num_resources+1, false, true);

    int* response = read_compound(socket);
    print_comm(response, response[1] + 2, true, false);

    close(socket);

    for (unsigned int request_id = 0; request_id < num_request_per_client; request_id++) {


        int *request = safeMalloc((num_resources + 1) * sizeof(int));
        request[0] = ct->id;
        for (int i = 0; i < num_resources; i++) {

            if(i == num_resources-1) {                          //si derniere REQ
                request[i + 1 ] = -(ct->used_ressources[i]);    //libere tout ce qu'on avait

            } else {
                int pos = (random_bounded(2));  //de 0 a 2-1

                if(pos) request[i + 1] = random_bounded(ct->max_ressources[i]+1);   //de 0 a (max de ressource i +1)-1
                else request[i + 1] = random_bounded(ct->used_ressources[i]+1);     //de 0 a (used i +1)-1
            }
        }

        int head[2] = {REQ, num_resources+1};

        fprintf(stdout, "Client %d is preparing its %d request\n", ct->id, request_id);

        send_request(ct, head, request);

        free(request);

        /* Attendre un petit peu (0s-0.1s) pour simuler le calcul.  */
        usleep(random() % (100 * 1000));
        /* struct timespec delay;
         * delay.tv_nsec = random () % (100 * 1000000);
         * delay.tv_sec = 0;
         * nanosleep (&delay, NULL); */
    }

    //ici, on a fait toutes nos request et on en est a fermer les clients
    ct_end(ct);

    pthread_exit(NULL);
}


//
// Vous devez changer le contenu de cette fonction afin de régler le
// problème de synchronisation de la terminaison.
// Le client doit attendre que le serveur termine le traitement de chacune
// de ses requêtes avant de terminer l'exécution.
//
void ct_wait_server(int num_clients, client_thread* client_threads) {

    while(true) {
        sleep(5);
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

    write_socket(socket, end, 2* sizeof(int), TIMEOUT, 0);
    print_comm(end, 2, true, true);

    int ack[2] = {-1, -1};  //TODO ceci est mauvais, changer pour int* et read_compound si erreur
    read_socket(socket, ack, 2* sizeof(int), TIMEOUT);
    print_comm(ack, 2, true, true);
}

void ct_create_and_start(client_thread *ct) {
    ct->id = count++;   //provient de ct_init()
    ct->max_ressources = malloc(num_resources* sizeof(int));
    ct->used_ressources = malloc(num_resources*sizeof(int));

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

    printf("connecting to server... ");
    while (connect(socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0);

    return socket_fd;
}
