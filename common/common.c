#include "common.h"

ssize_t read_socket(int sockfd, void *buf, size_t obj_sz, int timeout) {
    if (obj_sz == 0) return 1;  //succesfully read 0 bytes

    int ret;
    int len = 0;

    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN | POLLPRI;
    fds[0].revents = 0;

    do {
        // wait for data or timeout
        ret = poll(fds, 1, timeout);    //-1 si erreur, 0 si timeout, >0 sinon

        if (ret > 0) {
            if (fds->revents & POLLIN) {
                ret = recv(sockfd, (char *) buf + len, obj_sz - len, 0);
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

void *safeMalloc(size_t s) {
    void *temp = malloc(s);
    if (temp == NULL) {
        fprintf(stdout, "MALLOC ERROR");
        exit(1);
    }
    return temp;
}

int* read_compound(int socket_fd) {
    int head[2] = {-1, -1};
    read_socket(socket_fd, head, sizeof(head), TIMEOUT);

    int* cmd_receiver = malloc(head[1]* sizeof(int));
    if(cmd_receiver)
    read_socket(socket_fd, cmd_receiver, head[1]* sizeof(int), TIMEOUT);    //TODO ceci plante...

    int real_com_size = head[1]+2;
    int* real_com = malloc(real_com_size* sizeof(int));
    real_com[0] = head[0];
    real_com[1] = head[1];
    for(int i=0; i<head[1]; i++) {
        real_com[i+2] = cmd_receiver[i];
    }

    free(cmd_receiver);
    return real_com;
}

ssize_t write_socket(int sockfd, void *buf, size_t obj_sz, int timeout, int flag) {
    if (obj_sz == 0) return 1;  //succesfully read 0 bytes

    int ret;
    int len = 0;

    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLOUT | POLLPRI;
    fds[0].revents = 0;

    do {
        // wait for data or timeout
        ret = poll(fds, 1, timeout);    //-1 si erreur, 0 si timeout, >0 sinon

        if (ret > 0) {
            if (fds->revents & POLLOUT) {
                ret = send(sockfd, (char *) buf + len, obj_sz - len, flag);
                if (ret < 0) {
                    // abort connection
                    perror("send()");
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

void write_compound(int socket, int head[2], int* message) {
    int message_size = head[1];

    write_socket(socket, head, 2 * sizeof(int), TIMEOUT, (message_size==0? 0: MSG_MORE));
    if(message_size == 0) return;

    write_socket(socket, message, message_size * sizeof(int), TIMEOUT, 0);
}

void print_comm(int *arr, int size, bool print_enum, bool print_n) {
    if (print_enum) {
        char *temp;

        switch (arr[0]) {
            case 0:
                temp = "BEGIN";
                break;
            case 1:
                temp = "CONF";
                break;
            case 2:
                temp = "INIT";
                break;
            case 3:
                temp = "REQ";
                break;
            case 4:
                temp = "ACK";
                break;
            case 5:
                temp = "WAIT";
                break;
            case 6:
                temp = "END";
                break;
            case 7:
                temp = "CLO";
                break;
            case 8:
                fprintf(stderr, "ERR %d ", arr[1]);
                for (int i = 2; i < size; i++) {
                    fprintf(stderr, "%c", arr[i]);
                }
                fprintf(stderr, "\n");
                return;
            case 9:
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