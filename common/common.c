#include "common.h"

/**
 * Code pour les operations sur les sockets. Tire et modifie du code de read_socket donne avec ce TP par les
 * demonstrateurs.
 *
 * @param read      est-ce qu'on lit ou ecrit?
 * @param sockfd    le socket
 * @param buf       le buffer de lecture/d'ecriture
 * @param obj_sz    la grosseur du buffer
 * @param flags     les flags de l'operation
 * @return          le nombre de choses ecrites/lues
 */
ssize_t op_socket_code(bool read, int sockfd, void *buf, size_t obj_sz, int flags) {
    if (obj_sz == 0) return 1;  //succesfully read 0 bytes

    int ret;
    int len = 0;

    struct pollfd fds[1];
    fds[0].fd = sockfd;

    if(read) fds[0].events = POLLIN | POLLPRI;
    else fds[0].events = POLLOUT | POLLPRI;

    fds[0].revents = 0;

    do {
        // wait for data or timeout
        ret = poll(fds, 1, TIMEOUT);    //-1 si erreur, 0 si timeout, >0 sinon

        if (ret > 0) {
            if ((read ? fds->revents & POLLIN : fds->revents & POLLOUT)) {

                if(read) ret = recv(sockfd, (char *) buf + len, obj_sz - len, flags);
                else ret = send(sockfd, (char *) buf + len, obj_sz - len, flags);

                if (ret < 0) {
                    // abort connection
                    perror("recv()");
                    return -1;
                }
                len += ret;
            }
        } else {
            // TCP error or timeout
            if (ret < 0) {
                perror("poll()");
            }
            break;
        }
    } while (ret != 0 && len < obj_sz);
    return ret;
}

/**
 * Error handling pour op_socket_code. Tire/inspire de https://studium.umontreal.ca/mod/forum/discuss.php?d=641124
 *
 * Si il y a eu un probleme de lecture/ecriture, on imprime une erreur et on exit.
 *
 * @param read      est-ce qu'on lit ou ecrit?
 * @param sockfd    le socket
 * @param buf       le buffer de lecture/d'ecriture
 * @param obj_sz    la grosseur du buffer
 * @param flags     les flags de l'operation
 */
void err_h_socket(bool read, int sockfd, int *buf, size_t obj_sz, int flags) {

    int len = op_socket_code(read, sockfd, buf, obj_sz, flags);

    if (len > 0) {
        if (len != obj_sz) {
            fprintf(stderr, "Received invalid command size=%d!\n", len);
            exit(3);
        } else {
            return;
        }
    } else {
        if (len == 0) {
            fprintf(stderr, "Connection timeout\n");
            exit(30);
        }
    }
}

/**
 * Lit un socket
 *
 * @param sockfd    le socket
 * @param buf       le buffer de lecture
 * @param obj_sz    la grosseur du buffer
 */
void read_socket(int sockfd, int *buf, size_t obj_sz) {
    err_h_socket(true, sockfd, buf, obj_sz, 0); //quand on lit on a toujours pas de flags
}

/**
 * Ecrit sur un socket
 *
 * @param sockfd    le socket
 * @param buf       le buffer d'ecriture
 * @param obj_sz    la grosseur du buffer
 * @param flags     les flags
 */
void write_socket(int sockfd, void *buf, size_t obj_sz, int flags) {
    err_h_socket(false, sockfd, buf, obj_sz, flags);
}

/**
 * Lit une commande composite:
 *  1. Lit les deux premiers int dans head[2]
 *  2. malloc un int* de la grosseur de head[1] (puisqu'on recoit CMD NB_ARGS ARG1 ARG2...ARGN)
 *  3. Lit les prochains int dans ce int*
 *  4. Colle le head et le int* dans un gros int* qu'on retourne
 *
 * @param socket_fd le socket
 * @return          la commande lue
 */
int* read_compound(int socket_fd) {
    int head[2] = {-1, -1};
    read_socket(socket_fd, head, sizeof(head));

    int real_com_size = head[1]+2;
    int* real_com = malloc(real_com_size* sizeof(int));
    real_com[0] = head[0];
    real_com[1] = head[1];

    if(head[1] > 0) {   //s'il reste des choses a lire
        int* cmd_receiver = safeMalloc(head[1]* sizeof(int));   //int* de grosseur du reste de la commande
        read_socket(socket_fd, cmd_receiver, head[1]* sizeof(int));

        for(int i=0; i<head[1]; i++) {
            real_com[i+2] = cmd_receiver[i];
        }

        free(cmd_receiver);
    };

    print_comm(real_com, real_com[1]+2, true, true);
    return real_com;
}

/**
 * Ecrit une commande composite
 *
 * 1. Ecrit la tete de la commane
 * 2. Ecrit le corps
 *
 * @param socket    le socket
 * @param head      la tete de message de forme [CMD, nbArgs]
 * @param message   le corps du message
 */
void write_compound(int socket, int head[2], int* message) {
    int message_size = head[1];

    //flag pour plus s'assure que la connection est plus stable
    write_socket(socket, head, 2 * sizeof(int), (message_size==0? 0: MSG_MORE));

    if(message_size == 0) return;

    write_socket(socket, message, message_size * sizeof(int), 0);

    print_comm(head, 2, true, false);
    print_comm(message, head[1]+1, false, true);
}

/**
 * Malloc "safe" dans le sens que si il echoue on exit le programme
 *
 * @param s la grosseur du pointeur a allouer
 * @return  le pointeur alloue
 */
void *safeMalloc(size_t s) {
    void *temp = malloc(s);
    if (temp == NULL) {
        fprintf(stderr, "CRITICAL MALLOC ERROR");
        exit(1);
    }
    return temp;
}

/**
 * Imprime une commande/array de int
 *
 * @param arr           le array a imprimer
 * @param size          la grosseur du array
 * @param print_enum    est-ce qu'on print le premier int comme une commande?
 * @param print_n       est-ce qu'on imprime un newline apres?
 */
void print_comm(int *arr, int size, bool print_enum, bool print_n) {
    if (print_enum) {
        char *temp;

        switch (arr[0]) {
            //deux premiers cas utiles juste pour bankers
            case ERR_NEGA:
                fprintf(stdout, "ERR_NEGA");
                if(print_n) fprintf(stdout, "\n");
                return;
            case ERR_NEEDED:
                fprintf(stdout, "ERR_NEEDED");
                if(print_n) fprintf(stdout, "\n");
                return;
            //cas de l'enum des commanes
            case BEGIN:
                temp = "BEGIN";
                break;
            case CONF:
                temp = "CONF";
                break;
            case INIT:
                temp = "INIT";
                break;
            case REQ:
                temp = "REQ";
                break;
            case ACK:
                temp = "ACK";
                break;
            case WAIT:
                temp = "WAIT";
                break;
            case END:
                temp = "END";
                break;
            case CLO:
                temp = "CLO";
                break;
            case ERR:
                fprintf(stderr, "ERR %d ", arr[1]);
                for (int i = 2; i < size; i++) {    //imprime les int comme s'ils etaient des char (safe)
                    fprintf(stderr, "%c", arr[i]);
                }
                if(print_n) fprintf(stderr, "\n");
                return;
            case NB_COMMANDS:
                temp = "NB_COMMANDS";
                break;
            default:
                exit(1);
        }
        fprintf(stdout, "%s ", temp);
    } else {
        fprintf(stdout, "%i ", arr[0]);
    }

    for (int i = 1; i < size; i++) {
        fprintf(stdout, "%i ", arr[i]);
    }

    if (print_n) fprintf(stdout, "\n");
}