#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ulimit.h>
#include <string.h>
#include <errno.h>
#include <linux/seccomp.h>
#include <linux/filter.h>
#include <sys/prctl.h>
#include <linux/bpf.h>
#include <sys/syscall.h>

#define GiB (1024 * 1024 * 1024)

pid_t child_pid;

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

int get_diff_timespec_up(struct timespec start, struct timespec end) {

    int diff;

    struct timespec res = time_diff_timespec(start, end);

    diff = (int) ceil(res.tv_sec * 1e3 + res.tv_nsec / 1e6);

    return diff == 0 ? 1 : diff; //getrusage returns 0 if time is very small
}

int get_diff_timespec_round(struct timespec start, struct timespec end) {

    int diff;

    struct timespec res = time_diff_timespec(start, end);

    diff = (int) round(res.tv_sec * 1e3 + res.tv_nsec / 1e6);

    return diff;
}


int status;
struct rusage usage;
struct timespec start, end;
struct timespec start_stop, end_stop;

FILE *file_time;
FILE *file_cpu_time;
FILE *file_physical_memory;

void create_all_files(int time, int cpu_time, int physical_memory) { 

    file_time = fopen(getenv("TESTPATH_TIME"), "w");

    if (file_time == NULL) {

        exit(6);
        
    }

    fprintf(file_time, "%d", time);
    fclose(file_time);

    file_cpu_time = fopen(getenv("TESTPATH_CPU_TIME"), "w");

    if (file_cpu_time == NULL) {

        exit(6);
        
    }

    fprintf(file_cpu_time, "%d", cpu_time);
    fclose(file_cpu_time);

    file_physical_memory = fopen(getenv("TESTPATH_PHYSICAL_MEMORY"), "w");

    if (file_physical_memory == NULL) {

        exit(6);
        
    }

    fprintf(file_physical_memory, "%d", physical_memory);
    fclose(file_physical_memory);
}

void handle_timeout(int signum) {

    kill(child_pid, 9);
    waitpid(child_pid, NULL, 0);

    getrusage(RUSAGE_CHILDREN, &usage);

    long cpu_time = usage.ru_utime.tv_sec * 1000 + usage.ru_utime.tv_usec / 1000;

    create_all_files(atoi(getenv("REAL_TIME_LIMIT")) * 1000, cpu_time, usage.ru_maxrss);

    exit(2); //time limit

}


int main(int argc, char **argv) {

    signal(SIGALRM, handle_timeout);
    alarm(atoi(getenv("REAL_TIME_LIMIT"))); //just_time limit

    child_pid = fork();

    struct rlimit VMlimit = {2L * 1024 * 1024 * 1024, 2L * 1024 * 1024 * 1024}; //Global virtual memory limit
    struct rlimit SLimit; //Global stack memory limit

    SLimit.rlim_cur = 1024 * 1024 * 1024;

    if (child_pid == 0) {


        /*-------------------------------child process-------------------------------*/
        
        setrlimit(RLIMIT_AS, &VMlimit); //Virtual memory
        setrlimit(RLIMIT_STACK, &SLimit); //stack memory

        if(setuid(65534) < 0) { //failed to set user-nobody
            exit(6); 
        }


        execv(argv[0], argv); //execute user_program

    } else if (child_pid > 0) {

        /*-------------------------------parent process-------------------------------*/
        

        clock_gettime(CLOCK_MONOTONIC_RAW, &start);

        waitpid(child_pid, &status, 0);

        clock_gettime(CLOCK_MONOTONIC_RAW, &end);

        alarm(0); //off lust_time limit

        getrusage(RUSAGE_CHILDREN, &usage);

        if (!WIFEXITED(status)) {

            create_all_files(0, 0, 0);
            exit(6); //server error

        }

        if (WEXITSTATUS(status) != 0) { //runtime error
            
            create_all_files(0, 0, 0);
            exit(4); //runtime error

        }

        int time = get_diff_timespec_up(start, end) - get_diff_timespec_round(start_stop, end_stop);

        int cpu_time = (int)ceil(usage.ru_utime.tv_sec * 1000 + usage.ru_utime.tv_usec / 1000);

        if (cpu_time == 0) {

            cpu_time = (int)ceil(usage.ru_stime.tv_sec * 1000 + usage.ru_stime.tv_usec / 1000);

        }

        if (cpu_time == 0) { //getrusage returns 0 if time is very small (pr < 1ms)

            cpu_time = 1;

        }

        create_all_files(time, cpu_time, usage.ru_maxrss);

        exit(0);

    } else {

        exit(6); //server error
        
    }

}