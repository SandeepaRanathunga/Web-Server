#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>

#define PORT 8000

char *content_type=NULL;
int server_socket, client_socket;
struct sockaddr_in sock_address;
int addrlen = sizeof(sock_address);

void errorAndExit(char *msg){
    perror(msg);
    exit(1);
}
void mountSocket(){
    server_socket=socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket<0){
        errorAndExit("Unable to mount the socket");
    }
}

void bindSocket(){
    if (bind(server_socket, (struct sockaddr *)&sock_address, sizeof(sock_address)) < 0){
        errorAndExit("Binding error occured");
    }
}

void listenSocket(){
    if (listen(server_socket, 10) < 0){
        errorAndExit("Listner error occured");
    }
}

void respondToClient(int fd_client, char *status_code, void *msg, int msg_length){
    char response[msg_length + 100];
    int response_length = sprintf(response,"%s\nConnection: close\nContent-Length: %d\nContent-type: %s\n\n",
                                  status_code,msg_length,content_type);
    memcpy(response + response_length, msg, msg_length);
    send(fd_client, response, response_length + msg_length, 0);  //send to the client_socket
}

void renderFile(int fd_client, char *file_name){
    char *entry;
    size_t buffer_size;
    long int offset=0L;
    FILE *file = fopen(file_name, "r");//open the requested file if available
    if (file != NULL){
        if (fseek(file, offset, SEEK_END) == 0)//setting the file stream to a given offset
        {                          
            buffer_size = ftell(file);
            entry = malloc(sizeof(char) * (buffer_size + 1));
            fseek(file, offset, SEEK_SET);
            fread(entry, sizeof(char), buffer_size, file);
            respondToClient(fd_client, "HTTP/1.1 200 OK", entry, buffer_size);
        }
    }
    else{
        char *error = "Requested file not found!";
        content_type = "text/html";
        respondToClient(fd_client, "HTTP/1.1 404 NOT FOUND", error, strlen(error));
    }
}

int main(int argc, char const *argv[]){
    char buffer[30000];
    char request_type[4];
    char file_path[50];

    //mounting the socket
    mountSocket();

    //configure address attributes
    sock_address.sin_family = AF_INET;
    sock_address.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_address.sin_port = htons(PORT);

    //The address that the server_socket is going to listen goes here
    bindSocket();
    listenSocket();

    while (1){
        printf("\nAwaiting for a connection on port %d -------\n",PORT);
         //while listening from the server_socket in port 8000 we sholud create new socket to communicate with the client
        int client_socket=accept(server_socket, (struct sockaddr *)&sock_address, (socklen_t *)&addrlen);
        if (client_socket<0){
            errorAndExit("Error in acepting client socket");
        }

        read(client_socket, buffer, 1024); //reads data previously written to a file
        sscanf(buffer, "%s %s", request_type, file_path);
        
        //extract the file name
        char *file_name = strtok(file_path, "/");
        printf("Requsested file : %s",file_name);
        //obtain the file type
        char *extension = strrchr(file_path, '.') + 1; 

        if (extension)
            content_type = extension;

        if (strcmp(request_type, "GET")==0 && strcmp(file_path, "/")==0){ //strcmp returns 0 if two strings are equal
            char *msg= "Connection received";
            content_type = "text/html";
            
            //responds to client socket
            respondToClient(client_socket, "HTTP/1.1 200 OK", msg, strlen(msg));
        }
        else{
            //renders the requested file
            renderFile(client_socket, file_name);
        }
        printf("\nResponse sent\n");
    }
    return 0;
}
