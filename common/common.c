#include "common.h"

ssize_t read_socket(int sockfd, void *buf, size_t obj_sz, int timeout) {
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
          ret = recv(sockfd, (char*)buf + len, obj_sz - len, 0);
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

void* safeMalloc(size_t s) {
    void* temp = malloc(s);
    if(temp == NULL) {
        fprintf(stdout, "MALLOC ERROR");
        exit(1);
    }
    return temp;
}

void print_comm(int *arr, int size, bool print_enum, bool print_n) {
    if(print_enum) {
        char* temp;

        switch(arr[0]) {
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
                temp = "ERR";
                break;
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

    for(int i=1; i<size; i++) {
        fprintf(stdout, "%i ", arr[i]);
    }

    if(print_n) fprintf(stdout, "\n");
}