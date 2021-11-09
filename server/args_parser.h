#ifndef ARGS_PARSER
#define ARGS_PARSER

struct server_args{
    int port;
    int rate_requests;
    int rate_seconds;
    int max_users;
    int time_out;
};

void parse_args(int argc, char *argv[], struct server_args *args);

#endif