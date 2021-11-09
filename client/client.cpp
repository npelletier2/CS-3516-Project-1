#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <iostream>
#include <string.h>
#include <unistd.h>

void send_file(FILE* msg, int sock){
    //get size of file
    struct stat st;
    fstat(fileno(msg), &st);
    off_t size = st.st_size;
    std::cout << "size of file: " << size << std::endl;

    //read file into buffer
    char buf[size];
    fread(buf, size, 1, msg);

    //send buffer
    off_t bytes_sent = 0;
    while(bytes_sent < size){
        bytes_sent += send(sock, buf, size, 0);
    }

    std::cout << "bytes sent: " << bytes_sent << std::endl;

    std::cout << "Sent message: " << msg << std::endl;
}

void receive_msg(char *msg, int buf_size, int sock){
    // TODO: trying to add this chunk of code to recieve from the socket totally stops the server for some reason
    char buf[buf_size + 1]; //buffer for incoming message parts
    memset(msg, 0, buf_size+1);
    memset(buf, 0, buf_size+1);
    
    //loop until entire message has been received or received message is larger than buf_size
    int bytes_received = 0;
    int tot_bytes_received = 0;
    while(tot_bytes_received < buf_size){
        std::cout << "calling recv" << std::endl;
        bytes_received = recv(sock, buf, buf_size, 0);
        std::cout << "bytes received: " << bytes_received << std::endl;
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
    if(argc == 2){
        ip = argv[1];
    }
    
    struct addrinfo hints; //contains information that is passed to getaddrinfo
    struct addrinfo *res; //holds the results of getaddrinfo
    int sock; //socket descriptor of the socket used to handle client-server interaction

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(ip, "2012", &hints, &res) != 0){
        std::cout << "getaddrinfo failure";
        exit(1);
    }

    if((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0){
        std::cout << "socket failure";
        exit(1);
    }

    if(connect(sock, res->ai_addr, res->ai_addrlen)){
        std::cout << "connect failure";
        exit(1);
    }

    //Communicating on socket sock

    FILE *msg = fopen("qr_testing/project1_qr_code.png", "rb");

    send_file(msg, sock);

    fclose(msg);

    int buf_size = 1023;
    char recv_msg[buf_size+1];
    receive_msg(recv_msg, buf_size, sock);

    close(sock);

    std::cout << "Message received:\n" << recv_msg << std::endl;

    return 0;
}