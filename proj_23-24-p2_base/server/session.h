typedef struct session_id session_id;

struct session_id {
    int id;
    int req_fd;
    int resp_fd;
};
