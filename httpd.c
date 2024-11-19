#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int validate_arguments(int argc, char* argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: ./httpd <port>\n");
        return 1;
    }else{
        uint32_t tmp = atoi(argv[1]);
        if(tmp<1024 || tmp>65535){
            fprintf(stderr, "Please only use ports 1024-65535\n");
            return 1;
        }
        return 0;
    }
}

int main(int argc, char* argv[]){
    validate_arguments(argc, argv);

    uint16_t port = atoi(argv[1]);
    return 0;
}