#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "session.h"

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"

/*
void *process_file()
*/

int my_func(session_id *session){
  char OP_CODE;
  int return_value;
  unsigned int event_id;
  size_t num_rows, num_cols, num_seats, xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
  while (1){
      read(session->req_fd, &OP_CODE, 1);
      switch (OP_CODE){
      case '2':
        /* pthread_exit */
        close(session->req_fd);
        close(session->resp_fd);
        free(session);
        return 0;
      case '3':
        read(session->req_fd, &event_id, sizeof(unsigned int));
        read(session->req_fd, &num_rows, sizeof(size_t));
        read(session->req_fd, &num_cols, sizeof(size_t));
        return_value = ems_create(event_id, num_rows, num_cols);
        write(session->resp_fd, &return_value, sizeof(int));
        break;
      case '4':
        read(session->req_fd, &event_id, sizeof(unsigned int));
        read(session->req_fd, &num_seats, sizeof(size_t));
        read(session->req_fd, xs, sizeof(xs));
        read(session->req_fd, ys, sizeof(ys));
        return_value = ems_reserve(event_id, num_seats, xs, ys);
        write(session->resp_fd, &return_value, sizeof(int));
        break;
      case '5':
        read(session->req_fd, &event_id, sizeof(unsigned int));
        return_value = ems_show(session->resp_fd, event_id);
        write(session->resp_fd, &return_value, sizeof(int));
        break;
      case '6':
        return_value = ems_list_events(session->resp_fd);
        write(session->resp_fd, &return_value, sizeof(int));
        break;
      }
    }
}

int main(int argc, char* argv[]) {
  int fserv, session_counter = 0;
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }

  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  //TODO: Intialize server, create worker threads
  session_id* sessions[MAX_SESSIONS * sizeof(session_id*)];
  unlink(argv[1]);
  if (mkfifo (argv[1], 0777) < 0) return 1;
  if ((fserv = open (argv[1], O_RDONLY | O_CREAT)) < 0){
    fprintf(stderr, "server pipe failed to open\n");
    return 1;
  }
  
  char path_to_client[sizeof("../client/") + PATH_SIZE];
  char aux[sizeof("../client/")] = "../client/";
  char req_path[PATH_SIZE];
  char resp_path[PATH_SIZE];

  while (1) {
    //TODO: Read from pipe
    while(session_counter == MAX_SESSIONS);
    // pthread_create
    strcpy(path_to_client, aux);
    sessions[session_counter] = malloc(sizeof(session_id));

    sessions[session_counter]->id = session_counter;
    read(fserv, req_path, PATH_SIZE);
    if ((sessions[session_counter]->req_fd = open (strcat(path_to_client, req_path), O_RDONLY)) < 0){
    fprintf(stderr, "request pipe of session: %d, failed to open\n", session_counter);
    return 1;
    }
    strcpy(path_to_client, aux);
    read(fserv, resp_path, PATH_SIZE);
    if ((sessions[session_counter]->resp_fd = open (strcat(path_to_client, resp_path), O_WRONLY)) < 0){
    fprintf(stderr, "response pipe of session: %d, failed to open\n", session_counter);
    return 1;
    }
    write(sessions[session_counter]->resp_fd, &session_counter, sizeof(int));
    //TODO: Write new client to the producer-consumer buffer
    my_func(sessions[session_counter]);
    
    session_counter++;
    break;
  }

  //TODO: Close Server
  close(fserv);
  ems_terminate();
  return 0;
}