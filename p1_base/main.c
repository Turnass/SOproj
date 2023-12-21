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
#include <pthread.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"

#define MAX_FILE_NAME 128
struct thread_info
{
  char dirp_name[MAX_FILE_NAME];
  int max_threads;
  int fd_out;
  int id;
};

int first_open = 1;

void *execute(void *args)
{
  unsigned int event_id, delay;
  size_t num_rows, num_columns, num_coords;
  size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
  int fd_in;
  int line = -1;
  int command;
  unsigned int wait_num = 0;
  int *barrier_flag;

  struct thread_info *thread = (struct thread_info *)args;

  barrier_flag = malloc(sizeof(int));
  *barrier_flag = 0;

  if(first_open)
    fd_in = open(thread->dirp_name, O_RDONLY);

  fflush(stdout);

  while (1)
  {
    line++;

    command = get_next(fd_in);

    switch (command)
    {
    case CMD_CREATE:
      if (parse_create(fd_in, &event_id, &num_rows, &num_columns) != 0)
      {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (line % thread->max_threads != thread->id)
      {
        break;
      }

      if (ems_create(event_id, num_rows, num_columns))
      {
        fprintf(stderr, "Failed to create event\n");
      }

      break;

    case CMD_RESERVE:
      num_coords = parse_reserve(fd_in, MAX_RESERVATION_SIZE, &event_id, xs, ys);

      if (num_coords == 0)
      {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (line % thread->max_threads != thread->id )
      {
        break;
      }

      if (ems_reserve(event_id, num_coords, xs, ys))
      {
        fprintf(stderr, "Failed to reserve seats\n");
      }

      break;

    case CMD_SHOW:
      if (parse_show(fd_in, &event_id) != 0)
      {
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (line % thread->max_threads != thread->id )
      {
        break;
      }
      
      if (ems_show(event_id, thread->fd_out))
      {
        fprintf(stderr, "Failed to show event\n");
      }

      break;

    case CMD_LIST_EVENTS:

      if (line % thread->max_threads != thread->id)
      {
        break;
      }

      if (ems_list_events(thread->fd_out))
      {
        fprintf(stderr, "Failed to list events\n");
      }

      break;

    case CMD_WAIT:
      if (parse_wait(fd_in, &delay, &wait_num) == -1)
      { // thread_id is not implemented
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (delay > 0)
      {
        if ((int)wait_num == 0 || (int)wait_num == thread->id + 1){
        printf("Waiting... \n");
        ems_wait(delay);
        }
      }

      break;

    case CMD_INVALID:

      if (line % thread->max_threads != thread->id)
      {
        break;
      }

      fprintf(stderr, "Invalid command. See HELP for usage\n");
      break;

    case CMD_HELP:

      if (line % thread->max_threads != thread->id)
      {
        break;
      }

      printf(
          "Available commands:\n"
          "  CREATE <event_id> <num_rows> <num_columns>\n"
          "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
          "  SHOW <event_id>\n"
          "  LIST\n"
          "  WAIT <delay_ms> [thread_id]\n" // thread_id is not implemented
          "  BARRIER\n"                     // Not implemented
          "  HELP\n");

      break;

    case CMD_BARRIER:
      *barrier_flag = 1;
      free(thread);
      pthread_exit(barrier_flag);

    case CMD_EMPTY:
      break;

    case EOC:
      close(fd_in);
      free(thread);
      pthread_exit(barrier_flag);
    }
  }
}

int main(int argc, char *argv[])
{
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
  char dirp_name[MAX_FILE_NAME], f_in_name[MAX_FILE_NAME];
  char *dot_pointer;
  DIR *dirp;
  struct thread_info **threads;
  struct dirent *dp;
  pid_t pid;
  int max_proc = atoi(argv[2]), n_proc = 0, status;
  int max_threads = atoi(argv[3]);
  int i;
  int fd_out;
  int *barrier_flag;
  int aux;
  pthread_t tids[sizeof(pthread_t) * (unsigned int)max_threads];

  if (argc > 4)
  {
    char *endptr;
    unsigned long int delay = strtoul(argv[4], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX)
    {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_ms))
  {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }
  dirp = opendir(argv[1]);
  if (!dirp)
  {
    perror("Error opening directory");
    return 1;
  }

  strcpy(dirp_name, argv[1]);

  threads = malloc(sizeof(struct thread_info *) * (unsigned int)max_threads);

  while ((dp = readdir(dirp)))
  {
    if (strcmp(strchr(dp->d_name, '.'), ".jobs") == 0)
    {
      if (n_proc == max_proc)
      {
        pid = wait(&status);
        if (pid == -1)
        {
          perror("Error waiting for child process");
          exit(-1);
        }
        printf("O processo com pid: %d terminou com status: %d.\n", pid, status);
        n_proc--;
      }
      pid = fork();
      if (pid == -1)
      {
        perror("Error forking");
        exit(-1);
      }
      else if (pid == 0)
      {
        strcat(dirp_name, "/");
        strcat(dirp_name, dp->d_name);
        strcpy(f_in_name, dirp_name);
        dot_pointer = strchr(dirp_name, '.');
        strcpy(dot_pointer, ".out");
        fd_out = open(dirp_name, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);

        while(1){
          for (i = 0; i < max_threads; i++)
          {
            threads[i] = malloc(sizeof(struct thread_info));

            threads[i]->id = i;
            
            strcpy(threads[i]->dirp_name, f_in_name);
            threads[i]->max_threads = max_threads;

            threads[i]->fd_out = fd_out;

              if (pthread_create(&tids[i], NULL, execute, (void *)threads[i]) != 0) {
                fprintf(stderr, "Error creating thread\n");
                return EXIT_FAILURE;  
              }
          }

          for (i = 0; i < max_threads; i++){
            pthread_join(tids[i], (void **)&barrier_flag);
            aux = *barrier_flag;
            free(barrier_flag);
          }          
          first_open = 0;
          
          if(aux)
            continue;
          break;
        }

        close(fd_out);
        exit(0);
      }
      else
      {
        n_proc++;
        strcpy(dirp_name, argv[1]);
      }
    }
  }

  while (n_proc > 0)
  {
    pid = wait(&status);
    if (pid == -1)
    {
      perror("Error waiting for child process");
      exit(-1);
    }
    printf("O processo com pid: %d terminou com status: %d.\n", pid, status);
    n_proc--;
  }

  ems_terminate();
  free(threads);
  closedir(dirp);
  return 0;
}