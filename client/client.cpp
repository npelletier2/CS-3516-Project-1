#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <iostream>
#include <string.h>
#include <unistd.h>

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

void send_file(FILE* msg, int sock){
    //get size of file
    struct stat st;
    fstat(fileno(msg), &st);
    unsigned int size = st.st_size;
    std::cout << "size of file: " << size << std::endl;

    //read file into buffer
    char buf[size];
    fread(buf, size, 1, msg);

    //make a buffer that includes the size of the file in front
    char tosend[size+sizeof(int)];
    memcpy(tosend, &size, sizeof(int));
    memcpy(tosend+sizeof(int), buf, sizeof(buf));

    //send tosend buffer
    int bytes_sent = 0;
    while(bytes_sent < size){
        bytes_sent += send(sock, tosend, size+sizeof(int), 0);
    }

    std::cout << "bytes sent: " << bytes_sent << std::endl;

    std::cout << "Sent message: " << msg << std::endl;
}

void receive_msg(char *msg, unsigned int *code, unsigned int buf_size, int sock){
    unsigned int recv_size;

    recv_wrapper(sock, (char *)code, sizeof(unsigned int), 0);

    recv_wrapper(sock, (char *)&recv_size, sizeof(recv_size), 0);

    buf_size = std::min(buf_size, recv_size); //prevents overflow

    char buf[buf_size + 1]; //buffer for incoming message parts
    memset(msg, 0, buf_size+1);
    memset(buf, 0, buf_size+1);
    
    //loop until entire message has been received or received message is larger than buf_size
    unsigned int bytes_received = 0;
    unsigned int tot_bytes_received = 0;
    while(tot_bytes_received < buf_size){
        bytes_received = recv(sock, buf, buf_size, 0);
        if(bytes_received < 0){//error with recv
            std::cout << "recv failure" << std::endl;
            exit(1);
        }
        if(bytes_received == 0){//message finished, end loop
            strcat(msg, "\0");
            break;
        }else{//append received message to msg
            int bytes_to_copy = std::min(bytes_received, buf_size-tot_bytes_received); //this prevents overflow of the msg buffer
            memcpy(msg+tot_bytes_received, buf, bytes_to_copy);
            tot_bytes_received += bytes_to_copy;
        }
    }
}

int main(int argc, char *argv[]){
    char *ip = "127.0.0.1";
    char *port = "2012";
    if(argc == 2){
        ip = argv[1];
    }else if(argc == 3){
        ip = argv[1];
        port = argv[2];
    }
    
    struct addrinfo hints; //contains information that is passed to getaddrinfo
    struct addrinfo *res; //holds the results of getaddrinfo
    int sock; //socket descriptor of the socket used to handle client-server interaction

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(ip, port, &hints, &res) != 0){
        std::cout << "getaddrinfo failure" << std::endl;
        exit(1);
    }

    if((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0){
        std::cout << "socket failure" << std::endl;
        exit(1);
    }

    if(connect(sock, res->ai_addr, res->ai_addrlen)){
        std::cout << "connect failure" << std::endl;
        exit(1);
    }

    //Communicating on socket sock

    FILE *msg = fopen("qr_testing/project1_qr_code.png", "rb");

    send_file(msg, sock);

    fclose(msg);

    int buf_size = 1023;
    char recv_msg[buf_size+1];
    unsigned int code;
    receive_msg(recv_msg, &code, buf_size, sock);

    close(sock);

    std::cout << "Message received:\nCode: " << code << "\nMessage:\n" << recv_msg << std::endl;

    return 0;
}