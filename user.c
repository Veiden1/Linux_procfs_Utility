#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

void help(){
    fprintf(stderr, "Usage: ./user structure_ID path_file\n "
                    "Supported structures ID: \n "
                    "0 - pci_dev\n "
                    "1 - block_device\n");
}

int main(int argc, char *argv[]){
    if (argc != 3 && argc != 2) help();
    
    char *p;
    errno = 0;
    long structure_ID = strtol(argv[1], &p, 10);
    if (*p != '\0' || errno != 0){
        fprintf(stderr, "Provided structure_ID must be number.\n");
        help();
        return 0;
    }
    
    if (argc < 3 && structure_ID == 1){
        fprintf(stderr, "Not enough arguments \n" );
        return 0;
    }
    if (argc > 3){
        fprintf(stderr, "Too many arguments \n" );
        return 0;
    }

    if (structure_ID !=0 && structure_ID !=1){
        fprintf(stderr, "Provided structure ID is not supported.\n");
        help();
        return 0;
    }

    char inbuf[4096];
    char outbuf[4096];
    
    FILE *kmod_args = fopen("/proc/lab/struct_info", "w");
    
    if (structure_ID == 0) fprintf(kmod_args, "%s %s", argv[1], "");
    if (structure_ID == 1) {
    	fprintf(kmod_args, "%s %s", argv[1], argv[2]);
    }
    
    fclose(kmod_args);
    
    if (structure_ID == 0){
        printf("pci_dev structure: \n\n");
    } else {
        printf("block_device structure data for path %s: \n\n", argv[2]);
    }

    char cmd[100];
    sprintf(cmd, "cat /proc/lab/struct_info");
    system(cmd);

    return 0;
}
