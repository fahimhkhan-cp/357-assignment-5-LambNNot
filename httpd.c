#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "net.h"

/*Returns 0 if good, 1 if bad*/
int validate_arguments(int argc, char* argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: ./httpd <port>\n");
        return -1;
    }else{
        uint32_t tmp = atoi(argv[1]);
        if(tmp<1024 || tmp>65535){
            fprintf(stderr, "Please only use ports 1024-65535\n");
            return -1;
        }
        return 0;
    }
}

/*Replies to client*/
void reply(int clientfd, int msg_size){
   //Write to client

}

int validate_request(char* line, int num){
    if(strncmp(line, "GET", 3)==0){ 
        printf("GET Request Detected\n");
    }
    else if(strncmp(line, "HEAD", 4)==0){
        printf("HEAD Request Detected\n");
    }else{
        return 1;
    }
    return 0;
}

/*Handles request from client*/
void handle_request(int nfd){
    //Open socket in buffer
    FILE *network = fdopen(nfd, "r+");
    if (network == NULL){
        perror("fdopen");
        close(nfd);
        return;
    }

    //Read from client
    char *line = NULL;      //Stores line read from socket
    size_t size;            //Allocated size of line
    ssize_t num;            //Stores bytes read
    num = getline(&line, &size, network); //Read one line from socket (client)

    //TODO: Add Handling logic
    if(validate_request(line, num)){
        perror("Request type not supported\n");
        return;
    }

    //Free & Close
    free(line);
    fclose(network);
}

/*Listens for connections to server*/
void run_service(int service_fd){
   while(1){
        int nfd = accept_connection(service_fd);
        if (nfd != -1){
            printf("Connection established\n");
            handle_request(nfd);
            printf("Connection closed\n");
        }
   }
}

int main(int argc, char* argv[]){
    if(validate_arguments(argc, argv)){
        return 1;
    }
    short port = atoi(argv[1]);

    printf("%d\n", port);

    printf("t\n");
    //Initialize server
    int server_fd = create_service(port);
    printf("t\n");
    if(server_fd == -1){
        perror("create_service");
        exit(1);
    }

    //Start listening
    printf("Listening on port: %d\n", port);
    run_service(server_fd);
    close(server_fd);
    return 0;
}
