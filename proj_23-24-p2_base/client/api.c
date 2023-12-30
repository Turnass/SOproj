#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "api.h"

int fserv;
int req;
int res;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  //TODO: create pipes and connect to the server
  unlink(req_pipe_path);
  unlink(resp_pipe_path);
  if (mkfifo (req_pipe_path, 0777) < 0) return 1;
  if (mkfifo (resp_pipe_path, 0777) < 0) return 1;

  if ((fserv = open (server_pipe_path, O_WRONLY)) < 0){
    fprintf(stderr, "server pipe failed to open\n");
    return 1;
  }
  write(fserv, req_pipe_path, sizeof(req_pipe_path));
  write(fserv, resp_pipe_path, sizeof(resp_pipe_path));

  if ((req = open (req_pipe_path, O_WRONLY)) < 0){
    fprintf(stderr, "request pipe failed to open\n");
    return 1;
  }

  if ((res = open (resp_pipe_path, O_RDONLY)) < 0){
    fprintf(stderr, "response pipe failed to open\n");
    return 1;
  }

  return 0;
}

int ems_quit(void) { 
  //TODO: close pipes
  return 1;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_list_events(int out_fd) {
  //TODO: send list request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}
