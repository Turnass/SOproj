#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "../common/io.h"
#include "api.h"

int fserv;
int req;
int resp;
int id;
int return_value;
char OP_CODE;

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

  if (write(fserv, req_pipe_path, sizeof(req_pipe_path)) < 0){
    fprintf(stderr, "failed to write to server\n");
    return 1;
  }

  if ((req = open (req_pipe_path, O_WRONLY)) < 0){
    fprintf(stderr, "request pipe failed to open\n");
    return 1;
  }

  if (write(fserv, resp_pipe_path, sizeof(resp_pipe_path)) < 0){
    fprintf(stderr, "failed to write to server\n");
    return 1;
  }

  if ((resp = open (resp_pipe_path, O_RDONLY)) < 0){
    fprintf(stderr, "response pipe failed to open\n");
    return 1;
  }

  if (read(resp, &id, sizeof(int)) < 0){
    fprintf(stderr, "failed to read from server\n");
    return 1;
  }

  return 0;
}

int ems_quit(void) { 
  //TODO: close pipes
  OP_CODE = '2';
  write(req, &OP_CODE, 1);
  if(close(req) < 0) return 1;
  if(close(resp) < 0) return 1;
  if(close(fserv) < 0) return 1;
  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)
  OP_CODE = '3';
  write(req, &OP_CODE, 1);
  write(req, &event_id, sizeof(unsigned int));
  write(req, &num_rows, sizeof(size_t));
  write(req, &num_cols, sizeof(size_t));
  read(resp, &return_value, sizeof(int));
  return return_value;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  OP_CODE = '4';
  write(req, &OP_CODE, 1);
  write(req, &event_id, sizeof(unsigned int));
  write(req, &num_seats, sizeof(size_t));
  write(req, xs, sizeof(size_t) * 256);
  write(req, ys, sizeof(size_t) * 256);
  read(resp, &return_value, sizeof(int));
  return return_value;
}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  OP_CODE = '5';
  write(req, &OP_CODE, 1);
  write(req, &event_id, sizeof(unsigned int));
  char out_str;
  while (out_str != '<'){
    read(resp, &out_str, 1);
    print_chr(out_fd, out_str);
  }
  
  read(resp, &return_value, sizeof(int));
  return return_value;
}

int ems_list_events(int out_fd) {
  //TODO: send list request to the server (through the request pipe) and wait for the response (through the response pipe)
  OP_CODE = '6';
  write(req, &OP_CODE, 1);
  read(resp, &return_value, sizeof(int));
  return return_value;
}
