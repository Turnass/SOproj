#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "session.h"
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"


int standby_sessions = 0;
int active_threads[MAX_SESSION_COUNT * sizeof(int)];
pthread_mutex_t session_mutex;
sem_t AvailableThreadsSem;
sem_t ClientReadySem;
session_id** sessions = NULL;

int process_requets(session_id session){
  char OP_CODE;
  int return_value;
  unsigned int event_id;
  size_t num_rows, num_cols, num_seats, xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
  while (1){
      pthread_sigmask();
      read(session.req_fd, &OP_CODE, 1);
      sleep(2);
      printf("opcode : %c, id : %d\n", OP_CODE, session.id);
      switch (OP_CODE){
      case '2':
        close(session.req_fd);
        close(session.resp_fd);
        return 0;
      case '3':
        read(session.req_fd, &event_id, sizeof(unsigned int));
        read(session.req_fd, &num_rows, sizeof(size_t));
        read(session.req_fd, &num_cols, sizeof(size_t));
        return_value = ems_create(event_id, num_rows, num_cols);
        write(session.resp_fd, &return_value, sizeof(int));
        break;
      case '4':
        read(session.req_fd, &event_id, sizeof(unsigned int));
        read(session.req_fd, &num_seats, sizeof(size_t));
        read(session.req_fd, xs, sizeof(xs));
        read(session.req_fd, ys, sizeof(ys));
        return_value = ems_reserve(event_id, num_seats, xs, ys);
        write(session.resp_fd, &return_value, sizeof(int));
        break;
      case '5':
        read(session.req_fd, &event_id, sizeof(unsigned int));
        return_value = ems_show(session.resp_fd, event_id);
        write(session.resp_fd, &return_value, sizeof(int));
        break;
      case '6':
        return_value = ems_list_events(session.resp_fd);
        write(session.resp_fd, &return_value, sizeof(int));
        break;
      default:
        printf("invalid op code\n");
      }
    }
}

void *consumer(){
  session_id session;
  int i, j;
  while (1){
  sem_wait(&AvailableThreadsSem);
  sem_wait(&ClientReadySem);
  pthread_mutex_lock(&session_mutex);
  for (i = 0; i < MAX_SESSION_COUNT; i++){
    if(active_threads[i] == 0){
      active_threads[i] = 1;
      session = *sessions[0]; //dequeing
      write(session.resp_fd, &i, sizeof(int));
      session.id = i;
      free(sessions[0]);

      standby_sessions--;
      for (j = 0; j < standby_sessions; j++){
        sessions[j] = sessions[j + 1];
      }
      sessions = realloc(sessions, sizeof(session_id*) * sizeof(standby_sessions));
      
      break;
    }
  }
  pthread_mutex_unlock(&session_mutex);
  process_requets(session);
  active_threads[i] = 0;
  sem_post(&AvailableThreadsSem);
  }
}
int main(int argc, char* argv[]) {
  int fserv;
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

  pthread_mutex_init(&session_mutex, NULL);
  sem_init(&AvailableThreadsSem, 0, MAX_SESSION_COUNT);
  sem_init(&ClientReadySem, 0, 0);

  unlink(argv[1]);
  if (mkfifo (argv[1], 0777) < 0) return 1;

  pthread_t worker_th[MAX_SESSION_COUNT];

  for (int i = 0; i < MAX_SESSION_COUNT; i++){
    active_threads[i] = 0;
    if(pthread_create(&worker_th[i], NULL, &consumer, NULL) != 0){
      fprintf(stderr, "failed to create worker thread");
    }
  }

  char path_to_client[sizeof("../client/") + PATH_SIZE];
  char aux[sizeof("../client/")] = "../client/";
  char req_path[PATH_SIZE];
  char resp_path[PATH_SIZE];

  while (1) {
    //TODO: Read from pipe

    if ((fserv = open (argv[1], O_RDONLY | O_CREAT)) < 0){
      fprintf(stderr, "server pipe failed to open\n");
      return 1;
    }

    pthread_mutex_lock(&session_mutex);
    signal(SIGUSR1, catchSIGUSR1);
    sessions = realloc(sessions, sizeof(session_id*) * sizeof(standby_sessions));
    sessions[standby_sessions] = malloc(sizeof(session_id));

    strcpy(path_to_client, aux);
    read(fserv, req_path, PATH_SIZE);
    sessions[standby_sessions]->req_fd = open (strcat(path_to_client, req_path), O_RDONLY);

    strcpy(path_to_client, aux);
    read(fserv, resp_path, PATH_SIZE);
    sessions[standby_sessions]->resp_fd = open (strcat(path_to_client, resp_path), O_WRONLY);
    close(fserv);

    standby_sessions++;
    pthread_mutex_unlock(&session_mutex);
    //TODO: Write new client to the producer-consumer buffer
    sem_post(&ClientReadySem);
  }

  //TODO: Close Server
  pthread_mutex_destroy(&session_mutex);
  sem_destroy(&AvailableThreadsSem);
  sem_destroy(&ClientReadySem);
  ems_terminate();
  return 0;
}