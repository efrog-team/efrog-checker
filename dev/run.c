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

int DEBUG = 0;

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
long VmPeak = 0;
struct rusage usage;
struct timespec start, end;
struct timespec start_stop, end_stop;

FILE *file_time;
FILE *file_cpu_time;
FILE *file_virtual_memory;
FILE *file_physical_memory;

void create_all_files(int time, int cpu_time, long virtual_memory, int physical_memory) { 

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

    file_virtual_memory = fopen(getenv("TESTPATH_VIRTUAL_MEMORY"), "w");

    if (file_virtual_memory == NULL) {

        exit(6);
        
    }

    fprintf(file_virtual_memory, "%ld", virtual_memory);
    fclose(file_virtual_memory);

    file_physical_memory = fopen(getenv("TESTPATH_PHYSICAL_MEMORY"), "w");

    if (file_physical_memory == NULL) {

        exit(6);
        
    }

    fprintf(file_physical_memory, "%d", physical_memory);
    fclose(file_physical_memory);
}

void signal_handler(int signum) {

    clock_gettime(CLOCK_MONOTONIC_RAW, &start_stop);

    char filename[256];
    snprintf(filename, sizeof(filename), "/proc/%d/status", child_pid);

    FILE *fp = fopen(filename, "r");
    
    if (fp == NULL) {

        clock_gettime(CLOCK_MONOTONIC_RAW, &end);

        if (strcmp(getenv("LANGUAGE"), "Python 3 (3.10)") == 0) {
            FILE *ferr = fopen(getenv("TESTPATH_STDERR"), "r");

            char line[1000];
            int memoryerror = 0;
            while (fgets(line, sizeof(line), ferr) != NULL) {
                if (strstr(line, "MemoryError") != NULL) {
                    memoryerror = 1;
                    break;
                }
            }

            fclose(ferr);

            if (!memoryerror) {

                create_all_files(0, 0, 0, 0);
                exit(4);

            }
        } else {

            waitpid(child_pid, &status, 0);

            if (!WIFSIGNALED(status)) { //статус выхода без сигнала - рантайм

                create_all_files(0, 0, 0, 0);
                exit(4);

            }

        }

        getrusage(RUSAGE_CHILDREN, &usage);

        int cpu_time = (int)ceil(usage.ru_utime.tv_sec * 1000 + usage.ru_utime.tv_usec / 1000);

        if (cpu_time == 0) {

            cpu_time = (int)ceil(usage.ru_stime.tv_sec * 1000 + usage.ru_stime.tv_usec / 1000);

        }

        if (cpu_time == 0) { //getrusage returns 0 if time is very small (pr < 1ms)

            cpu_time = 1;

        }
        create_all_files(get_diff_timespec_up(start, end), cpu_time, atoi(getenv("VIRTUAL_MEMORY_LIMIT")) * 1024, usage.ru_maxrss);
        exit(3);
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "VmPeak:")) {
            if (sscanf(line, "VmPeak: %ld kB", &VmPeak) == 1) {
                break;
            }
        }
    }

    fclose(fp);

    signal(SIGCHLD, SIG_DFL);

    kill(child_pid, SIGCONT);

    clock_gettime(CLOCK_MONOTONIC_RAW, &end_stop);
    
}

void handle_timeout(int signum) {

    signal_handler(0); // to parse VmPeak

    kill(child_pid, 9);
    waitpid(child_pid, NULL, 0);

    getrusage(RUSAGE_CHILDREN, &usage);

    long cpu_time = usage.ru_utime.tv_sec * 1000 + usage.ru_utime.tv_usec / 1000;

    create_all_files(atoi(getenv("REAL_TIME_LIMIT")) * 1000, cpu_time, VmPeak, usage.ru_maxrss);

    exit(2); //time limit

}

int main(int argc, char **argv) {

    signal(SIGALRM, handle_timeout);

    alarm(atoi(getenv("REAL_TIME_LIMIT")));

    /*struct sock_filter seccomp_filter[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, 4),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_exit, 0, 3),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_exit_group, 0, 2),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),

        Block file-related syscalls 
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_open, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),

        Default allow for all other syscalls 
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };

    struct sock_fprog prog = {
        .len = sizeof(seccomp_filter) / sizeof(seccomp_filter[0]),
        .filter = seccomp_filter,
    };*/



    child_pid = fork();
    struct rlimit limit = {atoi(getenv("VIRTUAL_MEMORY_LIMIT")) * 1024 * 1024, atoi(getenv("VIRTUAL_MEMORY_LIMIT")) * 1024 * 1024};
    struct rlimit rl;
    rl.rlim_cur = 16 * 1024 * 1024;
    /*prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);*/
    if (child_pid == 0) {


        /*-------------------------------child process-------------------------------*/
        
        setrlimit(RLIMIT_AS, &limit);
        setrlimit(RLIMIT_STACK, &rl);

        execv(argv[0], argv);

    } else if (child_pid > 0) {

        /*-------------------------------parent process-------------------------------*/
        
        signal(SIGCHLD, signal_handler);

        clock_gettime(CLOCK_MONOTONIC_RAW, &start);

        waitpid(child_pid, &status, 0);

        clock_gettime(CLOCK_MONOTONIC_RAW, &end);

        alarm(0);

        getrusage(RUSAGE_CHILDREN, &usage);

        if (!WIFEXITED(status)) {
            
            create_all_files(0, 0, 0, 0);
            exit(6); //server error

        }

        if (WEXITSTATUS(status) != 0) {
            
            create_all_files(0, 0, 0, 0);
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

        create_all_files(time, cpu_time, VmPeak, usage.ru_maxrss);

        exit(0);

    } else {

        exit(6); //server error
        
    }
}