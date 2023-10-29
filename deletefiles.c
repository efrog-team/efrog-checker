#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h> 
#include <math.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <errno.h>


#define MP_len 1000

int delete_files(int submission_id, int submission) { // if submission = 1 S_id else D_id
    
    char* cf_id_path = (char*)malloc(MP_len); //checker files path

    sprintf(cf_id_path, submission ? "checker_files/S_%d" : "checker_files/D_%d", submission_id);

    char* command = (char*)malloc(MP_len); //command to delete dir
    sprintf(command, "rm -rf %s", dir_path);

    if (system(command) == 0) {

        return 0;

    } else {

        printf("ERROR : failed to remove the dir");
        return 1;

    }
}