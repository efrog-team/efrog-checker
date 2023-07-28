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

struct timeval time_diff_timeval(struct timeval start, struct timeval end) {

    struct timeval temp;

    if ((end.tv_usec - start.tv_usec) < 0) {

        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_usec = 1e6 + end.tv_usec - start.tv_usec;

    } else {

        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_usec = end.tv_usec - start.tv_usec;

    }

    return temp;

}

int get_diff_timespec(struct timespec start, struct timespec end) {

    int diff;

    struct timespec res = time_diff_timespec(start, end);

    diff = (int)ceil(res.tv_sec * 1e3 + res.tv_nsec / 1e6);

    return diff;
}

int get_diff_timeval(struct timeval start, struct timeval end) {

    int diff;

    struct timeval res = time_diff_timeval(start, end);

    diff = (int)ceil(res.tv_sec * 1e3 + res.tv_usec / 1e3);

    return diff;
}

int getbytes(int num) {

    return (int)((floor(log10(num)) + 2));

}


int create_files(int submission_id, char *code, char *language) {

    const int path_length = 14 + getbytes(submission_id);
    char path[path_length]; 

    sprintf(path, "checker_files/%d", submission_id);
    int status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    if (status != 0) { //error

        if(DEBUG) printf("failed to create a dir\n");
        return 1;

    } 

    FILE  *file_code;
    
    if (strcmp(language, "Python 3 (3.10)") == 0) {

        char code_path[path_length + getbytes(submission_id) + 4]; // path_length + getbytes(submission_id) + 4 (/.py)
        sprintf(code_path, "%s/%d.py", path, submission_id);

        file_code = fopen(code_path, "w");

        if (file_code == NULL) {

            if(DEBUG) printf("file_code = NULL\n");
            return 1;

        }

        fprintf(file_code, "%s", code);
        fclose(file_code);

    } else if (strcmp(language, "C++ 17 (g++ 11.2)") == 0 || strcmp(language, "C 17 (gcc 11.2)") == 0) { //cpp and c
        bool cpp = strcmp(language, "C++ 17 (g++ 11.2)") == 0; //c++
        int code_path_length = path_length + getbytes(submission_id) + 5;
        char code_path[code_path_length]; 
        sprintf(code_path, cpp ? "%s/%d.cpp" : "%s/%d.c", path, submission_id);
        file_code = fopen(code_path, "w");

        if (file_code == NULL) {

            if(DEBUG) printf("file_code = NULL\n");
            return 1;

        }

        fprintf(file_code, "%s", code);
        fclose(file_code);

        char compile_command[26 +  2 * code_path_length]; //g++-11 -static -s main.cpp -o main len : 23 (26 including -lm) +  2 * code_path_length
        sprintf(compile_command, cpp ? "g++-11 -static -s %s -o %s/%d" : "gcc-11 -static -s %s -lm -o %s/%d", code_path, path, submission_id); 
        int status = system(compile_command); 
        if (status == 1) { 
            if (DEBUG) printf("failed to compile (cpp)\n"); //compilling error
        }

    } else {

        if (DEBUG) printf("unknown language\n");
        return 1;

    }

}

int delete_files(int submission_id) {
    const int path_length = 14 + getbytes(submission_id);

    char dir_path[path_length];
    sprintf(dir_path, "checker_files/%d", submission_id);

    char command[7 + path_length];
    sprintf(command, "rm -rf %s", dir_path);

    int result = system(command);

    if (result == 0) {

        return 0;

    } else {

        if (DEBUG) printf("failed to remove the dir");
        return 1;

    }
}

struct Result
{
    int status; /* 0 - Correct answer
                   1 - Wrong answer
                   2 - Compilation error
                   3 - Runtime error
                   4 - Time limit
                   5 - Memory limit
                   6 - Internal error */      
    int time; //ms
    int cpu_time;
    int memory;
    char *output;
    char *description;
};

int execute(
    const char *file, 
    char **args, 
    const char *testpath_input,
    const char *testpath_output,
    const char *code_path,
    struct Result *result) { 

    /*define for error:*/

    result->status = 6;
    result->time = 0;
    result->cpu_time = 0;
    result->memory = 0;
    result->output = "";
    result->description = "";

    int input_fd = open(testpath_input, O_RDONLY);

    if (input_fd == -1) {

        if (DEBUG) printf("failed to open input file");
        return 1;

    }

    int output_fd = open(testpath_output, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    if (output_fd == -1) {

        close(input_fd);
        if (DEBUG) printf("failed to open output file");
        return 1;

    }

    pid_t pid = fork();

    if (pid == 0) {

        /*-------------------------------child process-------------------------------*/

        // freopen(testpath_input, "r", stdin);
        // freopen(testpath_output, "w", stdout);

        dup2(input_fd, STDIN_FILENO);
        dup2(output_fd, STDOUT_FILENO);

        close(input_fd);
        close(output_fd);
                
        execv(file, args);

    } else if (pid > 0) {

        /*-------------------------------parent process-------------------------------*/

        close(input_fd);
        close(output_fd);

        struct rusage usage1, usage2;
        int status;
        struct timespec start, end;

        getrusage(RUSAGE_CHILDREN, &usage1);

        
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        waitpid(pid, &status, 0);
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);

        getrusage(RUSAGE_CHILDREN, &usage2);

        int time = get_diff_timespec(start, end);

        // int cpu_time = usage2.ru_utime.tv_usec / 1000 - usage1.ru_utime.tv_usec / 1000;
        int cpu_time = get_diff_timeval(usage1.ru_utime, usage2.ru_utime);
        int memory = usage2.ru_maxrss - usage1.ru_maxrss;

        //space to check for error types!!!

        result->time = time;
        result->cpu_time = cpu_time;
        result->memory = memory;

        return 0;

    } else {

        if (DEBUG) printf("failed to create child process");
        result->status = 6;
        result->time = 0;
        result->cpu_time = 0;
        result->memory = 0;
        result->output = "";
        result->description = "";
        return 1; //error
        
    }

}

struct Result *check_test_case(int submission_id, int test_case_id, char *language, char *input, char *solution) {

    struct Result *result = malloc(sizeof(struct Result));
    result->status = 6; // define for error
    result->time = 0; 
    result->cpu_time = 0;
    result->memory = 0;
    result->output = "";
    result->description = "";
    //struct Result result = {6, 0, 0, 0, "", ""}; // define for error

    char output[1000000] = "";

    const int path_length = 14 + getbytes(submission_id);
    const int testpath_input_length = path_length + getbytes(test_case_id) + 11;

    char testpath_input[testpath_input_length]; //..._input.txt 
    char testpath_output[testpath_input_length + 1]; //..._output.txt
    char testpath_solution[testpath_input_length + 3]; //..._solution.txt

    sprintf(testpath_input, "checker_files/%d/%d_input.txt", submission_id, test_case_id);
    sprintf(testpath_output, "checker_files/%d/%d_output.txt", submission_id, test_case_id);
    sprintf(testpath_solution, "checker_files/%d/%d_solution.txt", submission_id, test_case_id);

    FILE *file_output;
    FILE *file_input;
    FILE *file_solution;

    file_input = fopen(testpath_input, "w");

    if (file_input == NULL) {

        if(DEBUG) printf("file_input = NULL\n");
        return result;
        
    }

    fprintf(file_input, "%s", input);
    fclose(file_input);

    file_solution = fopen(testpath_solution, "w");

    if (file_solution == NULL) {

        if(DEBUG) printf("file_solution = NULL\n");
        return result;

    }

    fprintf(file_solution, "%s", solution);
    fclose(file_solution);

    file_output = fopen(testpath_output, "w");

    if (file_solution == NULL) {

        if(DEBUG) printf("file_output = NULL\n");
        return result;

    }

    if (strcmp(language, "Python 3 (3.10)") == 0) {

        char code_path[path_length + getbytes(submission_id) + 4]; // path_length + getbytes(submission_id) + 4 (/.py)
        sprintf(code_path, "checker_files/%d/%d.py", submission_id, submission_id);

        char *file = "/usr/bin/python3";
        char *args[] = {file, code_path, NULL};

        int exec_status = execute(
            file, 
            args, 
            testpath_input, 
            testpath_output, 
            code_path, 
            result);

        if (exec_status == 1) {
            if (DEBUG) printf("failed in child process");
            return result;
        }
        
    } else if (strcmp(language, "C++ 17 (g++ 11.2)") == 0 || strcmp(language, "C 17 (gcc 11.2)") == 0) {

        int code_path_length = path_length + getbytes(submission_id) + 1; // path_length + getbytes(submission_id) + 1 :(/) :)))))))))))))))))))))))))))))))))

        char code_path[code_path_length]; 
        sprintf(code_path, "checker_files/%d/%d", submission_id, submission_id);

        char *file = code_path;
        char *args[] = {file, NULL};

        int exec_status = execute(
                    file, 
                    args, 
                    testpath_input, 
                    testpath_output, 
                    code_path, 
                    result);

        if (exec_status == 1) {

            if (DEBUG) printf("failed in child process");
            return result;

        }

    } else {

        if (DEBUG) printf("unknown language");
        return result;

    }

    file_output = fopen(testpath_output, "r");
    file_solution = fopen(testpath_solution, "r");

    char output_buffer[1000000], solution_buffer[1000000];
    
    char *output_read = fgets(output_buffer, 1000000, file_output);
    
    char *solution_read = fgets(solution_buffer, 1000000, file_solution);
    
    int status = -1;

    while (output_read != NULL && solution_read != NULL) {

        for (int i = strlen(output_buffer) - 1; i >= 0; i--) {

            if (output_buffer[i] != '\n' && output_buffer[i] != ' ') { 

                output_buffer[i + 1] = '\0';
                break;
            }

        }

        for (int i = strlen(solution_buffer) - 1; i >= 0; i--) {

            if (solution_buffer[i] != '\n' && solution_buffer[i] != ' ') { 

                solution_buffer[i + 1] = '\0';
                break;
            }

        }

        strcat(output, output_buffer);
        strcat(output, "\n");

        if (strcmp(output_buffer, solution_buffer) != 0) {

            status = 1;
            break;
        }

        output_read = fgets(output_buffer, 1000000, file_output);
        solution_read = fgets(solution_buffer, 1000000, file_solution);

    }

    
    if (status == -1) {

        if (output_read != NULL || solution_read != NULL) {

            status = 1;

        } else {

            status = 0;

        }    
    }  

    while (fgets(output_buffer, 1000000, file_output) != NULL) {

        strcat(output, output_buffer);
        strcat(output, "\n");

    }

    fclose(file_solution);      
    pclose(file_output);

    result->status = status;
    result->output = output;

    return result;
    
}
      
int main() {

    DEBUG = 1;

    //create_files(12312365, "num = int(input())\nprint(f\"{num // 10} {num % 10}\")\n", "Python 3 (3.10)");
    create_files(12312365, "#include <iostream>\n\nusing namespace std;\n\nint main() {\n    int a;\n    cin >> a;\n    cout << a * a;\n    return 0;\n}", "C++ 17 (g++ 11.2)");
    struct Result *result = check_test_case(12312365, 123123, "C++ 17 (g++ 11.2)", "99", "9 9");
    delete_files(12312365);

    printf(
        "status: %d\noutput: %stime: %dms\ncpu_time: %dms\nmemory: %dKB\n", 
        result->status, 
        result->output, 
        result->time, 
        result->cpu_time, 
        result->memory);

    return 0;

}