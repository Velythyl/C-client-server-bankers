#include "common.h"

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

void read_socket(int sockfd, int *buf, size_t obj_sz) {
    err_h_socket(true, sockfd, buf, obj_sz, 0);
}

void write_socket(int sockfd, void *buf, size_t obj_sz, int flags) {
    err_h_socket(false, sockfd, buf, obj_sz, flags);
}

int* read_compound(int socket_fd) {
    int head[2] = {-1, -1};
    read_socket(socket_fd, head, sizeof(head));

    int real_com_size = head[1]+2;
    int* real_com = malloc(real_com_size* sizeof(int));
    real_com[0] = head[0];
    real_com[1] = head[1];

    if(head[1] > 0) {
        int* cmd_receiver = safeMalloc(head[1]* sizeof(int));
        read_socket(socket_fd, cmd_receiver, head[1]* sizeof(int));

        for(int i=0; i<head[1]; i++) {
            real_com[i+2] = cmd_receiver[i];
        }

        free(cmd_receiver);
    };

    print_comm(real_com, real_com[1]+2, true, true);
    return real_com;
}

void write_compound(int socket, int head[2], int* message) {
    int message_size = head[1];

    write_socket(socket, head, 2 * sizeof(int), (message_size==0? 0: MSG_MORE));
    if(message_size == 0) return;

    write_socket(socket, message, message_size * sizeof(int), 0);

    print_comm(head, 2, true, false);
    print_comm(message, head[1]+1, false, true);
}

void *safeMalloc(size_t s) {
    void *temp = malloc(s);
    if (temp == NULL) {
        fprintf(stderr, "CRITICAL MALLOC ERROR");
        exit(1);
    }
    return temp;
}

void print_comm(int *arr, int size, bool print_enum, bool print_n) {
    if (print_enum) {
        char *temp;

        switch (arr[0]) {\
            case ERR_NEGA:
                fprintf(stdout, "ERR_NEGA");
                if(print_n) fprintf(stdout, "\n");
                return;
            case ERR_NEEDED:
                fprintf(stdout, "ERR_NEEDED");
                if(print_n) fprintf(stdout, "\n");
                return;
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
                for (int i = 2; i < size; i++) {
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