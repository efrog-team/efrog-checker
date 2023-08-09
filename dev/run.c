#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/resource.h>
#include <fcntl.h>

int DEBUG = 0;

struct timespec time_diff_timespec(struct timespec start, struct timespec end) {

    struct timespec temp;

    if ((end.tv_nsec - start.tv_nsec) < 0) {

        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1e9 + end.tv_nsec - start.tv_nsec;

    } else {

        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;

    }

    return temp;

}

int get_diff_timespec(struct timespec start, struct timespec end) {

    int diff;

    struct timespec res = time_diff_timespec(start, end);

    diff = (int) ceil(res.tv_sec * 1e3 + res.tv_nsec / 1e6);

    return diff == 0 ? 1 : diff; //getrusage returns 0 if time is very small
}

int main(int argc, char **argv) {

    pid_t pid_empty = fork();

    if (pid_empty == 0) {

        exit(0);

    } else if (pid_empty > 0) {

        waitpid(pid_empty, NULL, 0);
        struct rusage usage_empty;
        getrusage(RUSAGE_CHILDREN, &usage_empty);

        pid_t pid = fork();

        if (pid == 0) {

            /*-------------------------------child process-------------------------------*/

            execv(argv[0], argv);

        } else if (pid > 0) {

            /*-------------------------------parent process-------------------------------*/

            struct rusage usage;
            int status;
            struct timespec start, end;

            clock_gettime(CLOCK_MONOTONIC_RAW, &start);

            waitpid(pid, &status, 0);

            getrusage(RUSAGE_CHILDREN, &usage);

            clock_gettime(CLOCK_MONOTONIC_RAW, &end);


            if (!WIFEXITED(status)) {
                
                exit(6); //server error

            }

            if (WEXITSTATUS(status) != 0) {

                exit(4); //runtime error

            }

            int time = get_diff_timespec(start, end);

            int cpu_time = (int)ceil(usage.ru_utime.tv_sec * 1000 + usage.ru_utime.tv_usec / 1000);

            if (cpu_time == 0) {

                cpu_time = (int)ceil(usage.ru_stime.tv_sec * 1000 + usage.ru_stime.tv_usec / 1000);

            }

            if (cpu_time == 0) { //getrusage returns 0 if time is very small (pr < 1ms)

                cpu_time = 1;

            }

            int memory = usage.ru_maxrss - usage_empty.ru_maxrss;

            //space to check for error types!!!

            FILE *file_time;
            FILE *file_cpu_time;
            FILE *file_memory;

            file_time = fopen(getenv("TESTPATH_TIME"), "w");

            if (file_time == NULL) {

                exit(1);
                
            }

            fprintf(file_time, "%d", time);
            fclose(file_time);

            file_cpu_time = fopen(getenv("TESTPATH_CPU_TIME"), "w");

            if (file_cpu_time == NULL) {

                exit(1);
                
            }

            fprintf(file_cpu_time, "%d", cpu_time);
            fclose(file_cpu_time);

            file_memory = fopen(getenv("TESTPATH_MEMORY"), "w");

            if (file_memory == NULL) {

                exit(1);
                
            }

            fprintf(file_memory, "%d", memory);
            fclose(file_memory);

            exit(0);

        } else {

            exit(1); //error
            
        }
    }
}
