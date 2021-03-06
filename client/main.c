#include "client_thread.h"

/**
 * Initie le serveur
 *
 * 1. Envoie BEGIN
 * 2. Envoie CONF
 * 3. Attend la reponse
 */
void init_server() {
    /*
     * On utilise pas read et write compound puisque le traitement est plus facile lorsqu'on a acces aux parties du
     * message
     */

    int init_socket = c_open_socket();

    int RNG = (int) random();

    int begin[3] = {BEGIN, 1, RNG};
    write_socket(init_socket, begin, sizeof(begin), 0);
    print_comm(begin, 3, true, true);

    int conf1[2] = {1, num_resources};
    write_socket(init_socket, conf1, sizeof(conf1), MSG_MORE);   //envoie CONF nb ressources_max
    //envoie le nb de chaque ressources_max
    write_socket(init_socket, provisioned_resources, num_resources * sizeof(int), 0);
    print_comm(conf1, 2, true, false);
    print_comm(provisioned_resources, num_resources, false, true);

    int ok[3] = {-1, -1, -1};
    read_socket(init_socket, ok, sizeof(ok));
    print_comm(ok, 3, true, true);

    close(init_socket);

    if (ok[0] == 4 && ok[1] == 1 && ok[2] == RNG) return;
    else {
        fprintf(stdout, "Error on server start, exiting");
        exit(1);
    }
}

int main(int argc, char *argv[argc + 1]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <port-nb> <nb-clients> <nb-requests> <resources>...\n",
                argv[0]);
        exit(1);
    }

    port_number = atoi(argv[1]);
    int num_clients = atoi(argv[2]);
    num_request_per_client = atoi(argv[3]);
    num_resources = argc - 4;

    //ressources_max init
    provisioned_resources = safeMalloc(num_resources * sizeof(int));
    for (unsigned int i = 0; i < num_resources; i++) {
        provisioned_resources[i] = atoi(argv[i + 4]);
        fprintf(stdout, "Provisioned ressource %i: ", i);
        fprintf(stdout, "%s\n", argv[i + 4]);
    }

    srandom(time(NULL));

    init_server();

    init_client();

    // thread init
    client_thread *client_threads = safeMalloc(num_clients * sizeof(client_thread));
    for (unsigned int i = 0; i < num_clients; i++) {
        ct_create_and_start(&(client_threads[i]));
    }

    ct_wait_server(num_clients, client_threads);

    free(client_threads);

    // Affiche le journal.
    ct_print_results(stdout, true);
    FILE *fp = fopen("client.log", "w");
    if (fp == NULL) {
        fprintf(stderr, "Could not print log");
        return EXIT_FAILURE;
    }
    ct_print_results(fp, false);
    fclose(fp);

    return EXIT_SUCCESS;
}