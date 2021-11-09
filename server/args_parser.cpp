#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "args_parser.h"

void parse_args(int argc, char *argv[], struct server_args *args){
    args->port = 2012;
    args->rate_requests = 3;
    args->rate_seconds = 60;
    args->max_users = 3;
    args->time_out = 80;
    
    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "PORT") == 0){
            int p = atoi(argv[++i]);
            if(p >= 2000 && p <= 3000){
                args->port = p;
            }else{
                printf("Port number must be between 2000 and 3000, inclusive. Given port: %d", p);
                exit(1);
            }
        }else if(strcmp(argv[i], "RATE") == 0){
            args->rate_requests = atoi(argv[++i]);
            args->rate_seconds = atoi(argv[++i]);
        }else if(strcmp(argv[i], "MAX_USERS") == 0){
            args->max_users = atoi(argv[++i]);
        }else if(strcmp(argv[i], "TIME_OUT") == 0){
            args->time_out = atoi(argv[++i]);
        }else{
            printf("Invalid argument '%s' at position %d\n", argv[i], i);
            exit(1);
        }
    }
}