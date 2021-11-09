#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include "args_parser.h"
#include <string.h>
#include <unistd.h>
#include <fstream>
#include <cstdio>
#include "qr_decoder.hpp"
#include <arpa/inet.h>
#include <time.h>
#include <vector>
#include <sys/wait.h>

//TODO actually use these arguments
struct server_args *args = (struct server_args *) malloc(sizeof(server_args));
std::ofstream serv_log("server.log", std::ios_base::app);

void admin_log(std::string msg, std::string from = "SERVER"){
    time_t now_time_t = time(0);
    struct tm time_struct = *localtime(&now_time_t);
    char now[80];
    strftime(now, sizeof(now), "%F %T ", &time_struct);
    serv_log << now << from << ": " << msg << std::endl;
}

//returns the number of bytes recieved into buf
size_t recv_wrapper(int sock, char *buf, size_t n, int flags){
    char temp[n];//buffer to store message parts
    
    //loop until entire message is received
    size_t bytes_received = 0;
    size_t tot_bytes_received = 0;
    while(tot_bytes_received < n){
        bytes_received = recv(sock, temp, n, flags);
        if(bytes_received < 0){//error with recv
            std::cout << "recv failure" << std::endl;
            exit(1);
        }
        if(bytes_received == 0){//message finished, end loop
            break;
        }else{//append received message to buf
            int bytes_to_copy = std::min(bytes_received, n-tot_bytes_received); //this prevents overflow of the buf buffer
            memcpy(buf+tot_bytes_received, temp, bytes_to_copy);
            tot_bytes_received += bytes_to_copy;
        }
    }

    return tot_bytes_received;
}

size_t send_wrapper(int sock, unsigned int code, const void *buf, size_t n, int flags){
    //create server message
    char tosend[n+2*sizeof(unsigned int)];
    memcpy(tosend, &code, sizeof(unsigned int));
    memcpy(tosend+sizeof(unsigned int), &n, sizeof(unsigned int));
    if(buf != NULL) memcpy(tosend+2*sizeof(unsigned int), buf, n);
    
    //send server message
    size_t bytes_sent = 0;
    while(bytes_sent < n+2*sizeof(unsigned int)){
        bytes_sent += send(sock, tosend, n+2*sizeof(unsigned int), flags);
    }

    return bytes_sent;
}

//Send the decoded qr code
void send_success(int sock, const char *buf, unsigned int n){
    send_wrapper(sock, 0, buf, n, 0);
}

//Send a failure message
void send_failure(int sock){
    send_wrapper(sock, 1, NULL, 0, 0);
}

//Send a message alerting a client about a timeout
void send_timeout(int sock){
    std::string msg = "The connection has timed out";
    send_wrapper(sock, 2, msg.c_str(), msg.length()+1, 0);
}

//Send a message alerting the client that the server is busy
void send_busy(int sock){
    std::string msg = "The server is busy";
    send_wrapper(sock, 2, msg.c_str(), msg.length()+1, 0);
}

//Send a message alerting a client about exceeding the rate limit
void send_rate_limit_exceeded(int sock){
    std::string msg = "Rate limit exceeded";
    send_wrapper(sock, 3, msg.c_str(), msg.length()+1, 0);
}

//Returns false if the socket has been closed by the client, true otherwise
bool client_connected(int sock, std::string ip){
    //wait until there is data to be read or timeout
    fd_set socks;
    struct timeval timeout;
    FD_ZERO(&socks);
    FD_SET(sock, &socks);
    timeout.tv_sec = args->time_out;
    timeout.tv_usec = 0;
    int select_rv = select(sock+1, &socks, NULL, NULL, &timeout);
    if(select_rv < 0){
        std::cout << "select error" << std::endl;
        exit(0);
    }
    if(select_rv == 0){//select timed out, return false to end connection
        send_timeout(sock);
        admin_log("The connection has timed out", ip);
        return false;
    }

    //Check if client has closed the connection
    char buf[8];
    if(recv(sock, buf, sizeof(buf), MSG_PEEK | MSG_DONTWAIT) == 0){
        return false;
    }

    return true;
}

void handle_client(int new_sock, std::string ip){
    int num_requests = 0;
    time_t request_reset_time = time(0);
    while(client_connected(new_sock, ip)){
        //get the size of the file
        unsigned int msg_size;
        recv_wrapper(new_sock, (char *)&msg_size, sizeof(msg_size), 0);

        //handle rate limiting (after first recv, since now is when we know a request has been received by the server)
        if(time(0) - request_reset_time > args->rate_seconds){
            num_requests = 0;
            request_reset_time = time(0);
        }
        num_requests++;
        if(num_requests > args->rate_requests){
            send_rate_limit_exceeded(new_sock);
            admin_log("Rate limit exceeded", ip);
            continue;
        }

        //max 100 KB
        if(msg_size > 102400){
            msg_size = 102400;
        }

        //get the qr image
        char msg[msg_size];
        size_t tot_bytes_received = recv_wrapper(new_sock, msg, msg_size, 0);

        //use PID for filename to ensure different filenames for different concurrent threads
        std::string qr_filename = "qr_images/" + std::to_string(getpid()) + ".png";
        
        //write received qr code image to the file
        std::ofstream qr_file(qr_filename, std::ios::binary);
        qr_file.write(msg, tot_bytes_received);
        qr_file.close();

        admin_log("Recieved message", ip);

        //decode the qr image
        std::string qr_decoded = decode_qr(qr_filename);

        //delete file used to store qr code image
        remove(qr_filename.c_str());

        admin_log("QR decoded", ip);

        //send the string from the qr code back to client
        send_success(new_sock, qr_decoded.c_str(), qr_decoded.size());

        admin_log("Decoded QR sent", ip);
    }
}

int main(int argc, char *argv[]){
    parse_args(argc, argv, args); //fill the args structure with command line args, or default values for args that aren't given
    
    struct addrinfo hints; //contains information that is passed to getaddrinfo
    struct addrinfo *res; //holds the results of getaddrinfo
    int listen_sock; //socket descriptor of the socket that is listening for incoming connections

    struct sockaddr_storage client_addr; //address info for the client
    socklen_t addr_size = sizeof(client_addr); //size of client_addr
    int new_sock; //socket descriptor of the socket used to handle client-server interaction (result of call to accept)

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    char portstr[5];
    sprintf(portstr, "%d", args->port);
    if(getaddrinfo(NULL, portstr, &hints, &res) != 0){
        std::cout << "getaddrinfo failure" << std::endl;
        exit(1);
    }

    if((listen_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0){
        std::cout << "socket failure" << std::endl;
        exit(1);
    }

    int yes = 1;
    if(setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1){
        std::cout << "setsockopt failure" << std::endl;
        exit(1);
    }

    if(bind(listen_sock, res->ai_addr, res->ai_addrlen) != 0){
        std::cout << "bind failure" << std::endl;
        exit(1);
    }

    if(listen(listen_sock, args->max_users) != 0){
        std::cout << "listen failure" << std::endl;
        exit(1);
    }

    admin_log("Listening on socket " + std::to_string(listen_sock));

    std::vector<pid_t> current_users;
    while(1){
        if((new_sock = accept(listen_sock, (struct sockaddr *)&client_addr, (socklen_t *)&addr_size)) == -1){
            std::cout << "accept failure" << std::endl;
            exit(1);
        }

        std::string client_ip = inet_ntoa(((struct sockaddr_in *)&client_addr)->sin_addr);
        if(current_users.size() >= args->max_users){
            send_busy(new_sock);
            close(new_sock);
            admin_log("Connection denied: server is busy", client_ip);
            continue;
        }else{
            admin_log("Connection accepted", client_ip);
        }

        pid_t fork_ret = fork();
        if(fork_ret == -1){
            std::cout << "fork failure" << std::endl;
            exit(1);
        }
        if(fork_ret != 0){//parent
            close(new_sock);
            current_users.push_back(fork_ret);
            for(int i = 0; i < current_users.size(); i++){
                if(waitpid(current_users[i], NULL, WNOHANG) > 0){//this process has ended
                    current_users.erase(current_users.begin() + i);
                }
            }
        }else{//child
            close(listen_sock);
            handle_client(new_sock, client_ip);
            close(new_sock);
            admin_log("Connection closed", client_ip);
            exit(0);
        }
    }
}