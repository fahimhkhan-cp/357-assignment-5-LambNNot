#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
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
void reply(int clientfd, char* content, int content_length){
    //Write 
    dprintf(clientfd, "HTTP/1.0 200 OK\r\n");
    dprintf(clientfd, "Content-Type: text/html\r\n");
    dprintf(clientfd, "Content-Length: %d\r\n", content_length);
    dprintf(clientfd, "\r\n");

    //Write to client
    printf("%s", content);
    if(content!=NULL){
        write(clientfd, content, content_length);
    }

}

void response_error(int clientfd, int code){
    char buffer[64];
    char error_message[1024];
    if(code==400){
        //Bad Request Error
        sprintf(buffer, "Bad Request");
    }else if(code==403){
        //Permission Denied Error
        sprintf(buffer, "Permission Denied");
    }else if(code == 404){
        //Not Found Error
        sprintf(buffer, "Not Found");
    }else if(code == 501){
        //Not Implemented
        sprintf(buffer, "Not Implemented");
    }else{
        //Internal Error (500)
        sprintf(buffer, "Bad Request");
    }
    sprintf(error_message, "<!DOCTYPE html><html><head><h1>ERROR %d:</h1></head><body><p>%s</p></body></html>", code, buffer);
    reply(clientfd, error_message, sizeof(error_message));
}

/*Handles GET request*/
int get_request(char* file_name, int client_fd){

    //Check if file exists
    if(access(file_name, F_OK)!=0){
        //Reply w/ ERROR 404
        fprintf(stderr, "File name not found\n");
        response_error(client_fd, 404);
        exit(1);
    }

    //Open File
    FILE *file = fopen(file_name, "r");
    if(file==NULL){
        //Reply w/ ERROR 500
        fprintf(stderr, "fopen failed\n");
        response_error(client_fd, 500);
        exit(1);
    }

    //Check if in cgi-like directory
    if(strncmp(file_name, "cgi-like", strlen("cgi-like"))==0){
        printf("CGI-LIKE DETECTED\n");
    }

    //Stat file
    printf("FN: %s\n", file_name);
    struct stat status;
    if((stat(file_name, &status))<0){
        //Reply w/ ERROR 500
        fprintf(stderr, "stat failed\n");
        response_error(client_fd, 500);
        exit(1);
    }

    //Read file & Store in content
    char content[1024] = {0};
    int content_length;
    char* line = NULL;
    size_t size = 0;
    ssize_t num;
    while((num = getline(&line, &size, file)) != EOF){
        content_length += num;
        strcat(content, line);
    }
    reply(client_fd, content, content_length);

    //Free & Close
    free(line);
    fclose(file);
    return 0;
}

/*Handles HEAD request*/
int head_request(char* file_name, int client_fd){

    //Check if file exists
    if(access(file_name, F_OK)!=0){
        //Reply w/ ERROR 404
        fprintf(stderr, "File name not found\n");
        response_error(client_fd, 404);
        exit(1);
    }

    //Open File
    FILE *file = fopen(file_name, "r");
    if(file==NULL){
        //Reply w/ ERROR 500
        fprintf(stderr, "fopen failed\n");
        response_error(client_fd, 500);
        exit(1);
    }

    //Stat file
    printf("FN: %s\n", file_name);
    struct stat status;
    if((stat(file_name, &status))<0){
        //Reply w/ ERROR 500
        fprintf(stderr, "stat failed\n");
        response_error(client_fd, 500);
        exit(1);
    }

    //Reply to client
    reply(client_fd, NULL, 0);
    
    return 0;
}

/*Returns G on GET, H on HEAD, E if invalid */
char validate_request(char* line, int num){
    if(strncmp(line, "GET", 3)==0){ 
        printf("GET Request Detected\n");
        return 'G';
    }
    else if(strncmp(line, "HEAD", 4)==0){
        printf("HEAD Request Detected\n");
        return 'H';
    }else{
        return 'E';
    }
}

/*Handles request from client*/
void handle_request(int nfd){
    //Open socket in buffer
    FILE *network = fdopen(nfd, "r+");
    if (network == NULL){
        //Reply w/ ERROR 500
        fprintf(stderr, "stat failed\n");
        response_error(nfd, 500);
        exit(1);
    }

    //Read from client
    char *line = NULL;      //Stores line read from socket
    char *line_cpy = line;  //Used to store original pointer location bc strsep is problematic
    size_t size = 0;        //Allocated size of line
    ssize_t num;            //Stores bytes read
    num = getline(&line, &size, network); //Read one line from socket (client)

    char request;           //Stores the request type
    if((request= validate_request(line, num)) == 'E'){
        //TODO: Reply w/ ERROR 501
        fprintf(stderr, "Request type not supported\n");
        response_error(nfd, 501);
        exit(1);
    }

    //Parse file name from request
    char *file_name;
    while((file_name = strsep(&line," ")) != NULL){
        if(file_name[0]=='/'){
            //Check for inappropriate files
            if(file_name[1] == '.'){
                //Reply with ERROR 403
                fprintf(stderr, "No perms\n");
                response_error(nfd, 403);
                exit(1);
            }
            printf("Requested File: %s\n", ++file_name); //Increment pointer to get rid of "/" in file_name, which breaks shit otherwise
            break;
        }
    }
    if(file_name==NULL){
        //Reply w/ Error 400
        fprintf(stderr, "Bad Req\n");
        response_error(nfd, 400);
        exit(1);
    }

    //Null-terminate file_name properly if last character is '\n'
    size_t length;
    length = strlen(file_name);
    if(file_name[length-1] == '\n'){
        file_name[length-1] = '\0';
    }else{
        file_name[length] = '\0';
    }

    //Check for favicon.ico specifically (It auto-requests for it idk)
    if((strcmp(file_name, "favicon.ico"))!=0){
        if(request == 'G'){
            //GET Request Handling
            get_request(file_name, nfd);
        }else{
            //HEAD Request Handling
            head_request(file_name, nfd);
        }
    }

    //Free & Close
    free(line_cpy);
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
    //Initialize server
    int server_fd = create_service(port);
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
