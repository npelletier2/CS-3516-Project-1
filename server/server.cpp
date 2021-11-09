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

//TODO actually use these arguments
struct server_args *args = (struct server_args *) malloc(sizeof(server_args));

//returns the number of bytes recieved into buf
int recv_wrapper(int sock, char *buf, size_t n, int flags){
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

void handle_client(int new_sock){
    //get the size of the file
    int msg_size;
    recv_wrapper(new_sock, (char *)&msg_size, sizeof(int), 0);

    //max 100 KB
    if(msg_size > 102400){
        msg_size = 102400;
    }

    //get the qr image
    char msg[msg_size];
    int tot_bytes_received = recv_wrapper(new_sock, msg, msg_size, 0);

    //use PID for filename to ensure different filenames for different concurrent threads
    std::string qr_filename = "qr_images/" + std::to_string(getpid()) + ".png";
    
    //write received qr code image to the file
    std::ofstream qr_file(qr_filename, std::ios::binary);
    qr_file.write(msg, tot_bytes_received);
    qr_file.close();

    std::cout << "DEBUG: Received message, stored in " << qr_filename << std::endl;

    //decode the qr image
    std::string qr_decoded = decode_qr(qr_filename);

    //delete file used to store qr code image
    remove(qr_filename.c_str());

    std::cout << "DEBUG: " << qr_filename << " deleted" << std::endl;

    //send the string from the qr code back to client
    off_t bytes_sent = 0;
    while(bytes_sent < qr_decoded.size()){
        bytes_sent += send(new_sock, qr_decoded.c_str(), qr_decoded.size(), 0);
    }
}

int main(int argc, char *argv[]){
    parse_args(argc, argv, args); //fill the args structure with command line args, or default values for args that aren't given
    
    struct addrinfo hints; //contains information that is passed to getaddrinfo
    struct addrinfo *res; //holds the results of getaddrinfo
    int listen_sock; //socket descriptor of the socket that is listening for incoming connections

    struct sockaddr_storage client_addr; //address info for the client
    socklen_t addr_size; //size of client_addr
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

    std::cout << "DEBUG: getaddrinfo complete" << std::endl;

    if((listen_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0){
        std::cout << "socket failure" << std::endl;
        exit(1);
    }

    std::cout << "DEBUG: Created socket " << listen_sock << std::endl;

    if(bind(listen_sock, res->ai_addr, res->ai_addrlen) != 0){
        std::cout << "bind failure" << std::endl;
        exit(1);
    }

    std::cout << "DEBUG: Bind to socket " << listen_sock << " successful, ip: " << inet_ntoa(((struct sockaddr_in *)res->ai_addr)->sin_addr) << std::endl;

    if(listen(listen_sock, args->max_users) != 0){
        std::cout << "listen failure" << std::endl;
        exit(1);
    }

    std::cout << "DEBUG: Listening on socket " << listen_sock << std::endl;

    //TODO: Make a new process when a client is accepted
    addr_size = sizeof(client_addr);
    if((new_sock = accept(listen_sock, (struct sockaddr *)&client_addr, (socklen_t *)&addr_size)) == -1){
        std::cout << "accept failure" << std::endl;
        exit(1);
    }

    std::cout << "DEBUG: Connection on socket " << listen_sock << " accepted" << std::endl;

    close(listen_sock);

    //Communicating on socket new_sock

    handle_client(new_sock);

    close(new_sock);

    return 0;
}