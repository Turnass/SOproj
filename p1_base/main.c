#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"

void execute(char dirp_name[128]){
    unsigned int event_id, delay;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
    int fd_in, fd_out;
    char *dot_pointer;

    dot_pointer = strchr(dirp_name, '.');

    if(strcmp(dot_pointer, ".out") == 0)
      return;
    
    fd_in = open(dirp_name, O_RDONLY);
    strcpy(dot_pointer, ".out");
    
    fd_out = open(dirp_name, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);

    fflush(stdout);

    while (1){
    switch (get_next(fd_in)) {
      case CMD_CREATE:
        if (parse_create(fd_in, &event_id, &num_rows, &num_columns) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_create(event_id, num_rows, num_columns)) {
          fprintf(stderr, "Failed to create event\n");
        }
        
        break;

      case CMD_RESERVE:
        num_coords = parse_reserve(fd_in, MAX_RESERVATION_SIZE, &event_id, xs, ys);

        if (num_coords == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_reserve(event_id, num_coords, xs, ys)) {
          fprintf(stderr, "Failed to reserve seats\n");
        }

        break;

      case CMD_SHOW:
        if (parse_show(fd_in, &event_id) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_show(event_id, fd_out)) {
          fprintf(stderr, "Failed to show event\n");
        }

        break;

      case CMD_LIST_EVENTS:
        if (ems_list_events(fd_out)) {
          fprintf(stderr, "Failed to list events\n");
        }

        break;

      case CMD_WAIT:
        if (parse_wait(fd_in, &delay, NULL) == -1) {  // thread_id is not implemented
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (delay > 0) {
          printf("Waiting...\n");
          ems_wait(delay);
        }

        break;

      case CMD_INVALID:
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        break;

      case CMD_HELP:
        printf(
            "Available commands:\n"
            "  CREATE <event_id> <num_rows> <num_columns>\n"
            "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
            "  SHOW <event_id>\n"
            "  LIST\n"
            "  WAIT <delay_ms> [thread_id]\n"  // thread_id is not implemented
            "  BARRIER\n"                      // Not implemented
            "  HELP\n");

        break;

      case CMD_BARRIER:  // Not implemented
      case CMD_EMPTY:
        break;

      case EOC:
        close(fd_in);
        close(fd_out);
        return;
    }
}
}

int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
  int max_proc = atoi(argv[2]), n_proc = 0, status;
  char dirp_name[128];
  DIR *dirp;
  struct dirent *dp;
  pid_t pid;

  if (argc > 3) {
    char *endptr;
    unsigned long int delay = strtoul(argv[3], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_ms)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }
  dirp = opendir(argv[1]);
  if (!dirp) {
      perror("Error opening directory");
      return 1;
  }
  
  strcpy(dirp_name, argv[1]);

  while ((dp = readdir(dirp))){
  if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0){
    if (n_proc == max_proc){
      pid = wait(&status);
      if (pid == -1){
        perror("Error waiting for child process");
        exit(-1);
      }
      printf("O processo com pid: %d terminou com status: %d.\n", pid, status);
      n_proc--;
    }
    pid = fork();
    if (pid == -1) {
      perror("Error forking");
      exit(-1);
    }
    else if(pid == 0){
      strcat(dirp_name, "/");
      strcat(dirp_name, dp->d_name);
      execute(dirp_name);
      ems_terminate();
      closedir(dirp);
      exit(0);
    }
    else{
      n_proc++;
      strcpy(dirp_name, argv[1]);
    }
  }
  }
  
  while (n_proc > 0){
    pid = wait(&status);
      if (pid == -1){
        perror("Error waiting for child process");
        exit(-1);
      }
      printf("O processo com pid: %d terminou com status: %d.\n", pid, status);
      n_proc--;
    }
  
  ems_terminate();
  closedir(dirp);
  return 0;
}