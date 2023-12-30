#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "session.h"

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"

int main(int argc, char* argv[]) {
  int fserv, session_counter;
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
  
  char path_to_client[sizeof("../client/")] = "../client/";
  while (1) {
    //TODO: Read from pipe
    read(fserv, sessions[session_counter]->req_path, PATH_SIZE)
    //TODO: Write new client to the producer-consumer buffer
    break;
  }

  //TODO: Close Server
  close(fserv);
  ems_terminate();
  return 0;
}