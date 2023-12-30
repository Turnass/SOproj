#define PATH_SIZE 40
#define MAX_SESSIONS 10

typedef struct session_id session_id;

struct session_id {
    int id;
    char req_path[PATH_SIZE];
    char resp_path[PATH_SIZE];
};
