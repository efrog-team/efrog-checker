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

int get_diff_timespec_up(struct timespec start, struct timespec end) {

    int diff;

    struct timespec res = time_diff_timespec(start, end);

    diff = (int) ceil(res.tv_sec * 1e3 + res.tv_nsec / 1e6);

    return diff == 0 ? 1 : diff; //getrusage returns 0 if time is very small
}

int get_diff_timeval(struct timeval start, struct timeval end) {

    int diff;

    struct timeval res = time_diff_timeval(start, end);

    diff = (int) ceil(res.tv_sec * 1e3 + res.tv_usec / 1e3);

    return diff == 0 ? 1 : diff; //getrusage returns 0 if time is very small
}

int getbytes(int num) {

    return (int)((floor(log10(num)) + 2));

}

struct CreateFilesResult {
    int status; /*
                0 - Successful
                5 - Compilation error
                6 - Server error 
                */
    char *description;
};


struct CreateFilesResult *create_files(int submission_id, char *code, char *language) {
    
    struct CreateFilesResult *result = malloc(sizeof(struct CreateFilesResult));

    result->status = 6; //define for server error
    result->description = "";

    const int path_length = 14 + getbytes(submission_id);
    char path[path_length]; 

    sprintf(path, "checker_files/%d", submission_id);
    int status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    if (status != 0) { //error

        if(DEBUG) printf("failed to create a dir\n");
        return result;

    } 

    FILE  *file_code;
    
    if (strcmp(language, "Python 3 (3.10)") == 0 || strcmp(language, "Node.js (20.x)") == 0) {  

        bool py = strcmp(language, "Python 3 (3.10)") == 0;

        char code_path[path_length + getbytes(submission_id) + 4]; // path_length + getbytes(submission_id) + 4 (/.py)
        sprintf(code_path, py ? "%s/%d.py" : "%s/%d.js", path, submission_id);

        file_code = fopen(code_path, "w");

        if (file_code == NULL) {

            if(DEBUG) printf("file_code = NULL\n");
            return result;

        }

        if (py) {
            fprintf(file_code, "%s", "import os, signal\n");
        } else {
            fprintf(file_code, "%s", "const { exec } = require('child_process');\n");
        }
        fprintf(file_code, "%s", code);
        if (py) {
            fprintf(file_code, "%s", "\nos.kill(os.getpid(), signal.SIGSTOP)");
        } else {
            fprintf(file_code, "%s", "\nprocess.kill(process.pid, 'SIGSTOP');");
        }
        fclose(file_code);

    } else if (strcmp(language, "C++ 17 (g++ 11.2)") == 0 || strcmp(language, "C 17 (gcc 11.2)") == 0) { //cpp and c

        bool cpp = strcmp(language, "C++ 17 (g++ 11.2)") == 0; //c++

        int code_path_length = path_length + getbytes(submission_id) + 5;

        char code_path_main[code_path_length + 5];

        sprintf(code_path_main, cpp ? "%s/%d_main.cpp" : "%s/%d_main.c", path, submission_id);
        FILE *file_code_main;
        file_code_main = fopen(code_path_main, "w");

        if (file_code_main == NULL) {

            if(DEBUG) printf("file_code_main = NULL\n");
            return result;

        }

        fprintf(file_code_main, "#include <signal.h>\n#include <stdlib.h>\nvoid segfhand(int sig) {\n    exit(1);\n}\nvoid __attribute__((constructor)) ctor() {\n    signal(SIGSEGV, segfhand);\n}\nvoid __attribute__((destructor)) dtor() {\n    kill(getpid(), SIGSTOP);\n}\n%s", code);
        fclose(file_code_main);

        char compile_command[40 + 2 * code_path_length]; //g++-11 -static -s main.cpp -o main len : 23 (26 including -lm) +  2 * code_path_length + 5 ( 2>&1)
        sprintf(compile_command, cpp ? "g++-11 -static -s %s 2>&1 -o %s/%d" : "gcc-11 -static -s %s 2>&1 -lm -o %s/%d", code_path_main, path, submission_id); 

        FILE *ferr = popen(compile_command, "r");

        char cerror_buffer[1000] = "";
        char cerror[10000] = "";

        while (fgets(cerror_buffer, sizeof(cerror_buffer), ferr) != NULL) {

            strcat(cerror, cerror_buffer);
            strcat(cerror, "\n");

        }

        strcat(cerror, "\0");

        if (WEXITSTATUS(pclose(ferr)) != 0) {


            if (DEBUG) printf("failed to compile (C or C++)\n");

            result->status = 5;
            result->description = cerror;

            return result;
        }

    } else {

        if (DEBUG) printf("unknown language\n");
        return result;

    }

    result->status = 0;

    return result;
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

struct TestResult
{
    int status; /* 0 - Correct answer
                   1 - Wrong answer
                   2 - Time limit
                   3 - Memory limit
                   4 - Runtime error
                   5 - Compilation error
                   6 - Server error */      
    int time; //ms
    int cpu_time;
    int virtual_memory;
    int physical_memory;
    char *description;
};

int execute(
    int submission_id,
    int test_case_id,
    char **args,
    struct TestResult *result,
    int real_time_limit,
    int virtual_memory_limit /*IN MiB*/,
    char *language) { 

    //define for error:

    result->status = 6;
    result->time = 0;
    result->cpu_time = 0;
    result->virtual_memory = 0;
    result->physical_memory = 0;

    const int path_length = 14 + getbytes(submission_id);
    const int testpath_length = path_length + getbytes(test_case_id);

    char testpath_input[testpath_length + 10]; //..._input.txt
    char testpath_output[testpath_length + 11]; //..._output.txt
    char testpath_error[testpath_length + 10]; //..._error.txt
    char testpath_time[testpath_length + 9]; //..._time.txt
    char testpath_cpu_time[testpath_length + 13]; //..._cpu_time.txt
    char testpath_virtual_memory[testpath_length + 19]; //..._virtual_memory.txt
    char testpath_physical_memory[testpath_length + 20]; //..._physical_memory.txt

    sprintf(testpath_input, "checker_files/%d/%d_input.txt", submission_id, test_case_id);
    sprintf(testpath_output, "checker_files/%d/%d_output.txt", submission_id, test_case_id);
    sprintf(testpath_error, "checker_files/%d/%d_stderr.txt", submission_id, test_case_id);
    sprintf(testpath_time, "checker_files/%d/%d_time.txt", submission_id, test_case_id);
    sprintf(testpath_cpu_time, "checker_files/%d/%d_cpu_time.txt", submission_id, test_case_id);
    sprintf(testpath_virtual_memory, "checker_files/%d/%d_virtual_memory.txt", submission_id, test_case_id);
    sprintf(testpath_physical_memory, "checker_files/%d/%d_physical_memory.txt", submission_id, test_case_id);

    char language_env[strlen(language) + 10];
    char testpath_time_env[2 * (testpath_length + 9) + 1];
    char testpath_cpu_time_env[2 * (testpath_length + 13) + 1];
    char testpath_virtual_memory_env[2 * (testpath_length + 19) + 1];
    char testpath_physical_memory_env[2 * (testpath_length + 20) + 1];

    char real_time_limit_env[15 + getbytes(real_time_limit)];
    char virtual_memory_limit_env[21 + getbytes(virtual_memory_limit)]; 
    char testpath_stderr_env[27 + testpath_length];

    sprintf(language_env, "LANGUAGE=%s", language);
    sprintf(testpath_time_env, "TESTPATH_TIME=%s", testpath_time);
    sprintf(testpath_cpu_time_env, "TESTPATH_CPU_TIME=%s", testpath_cpu_time);
    sprintf(testpath_virtual_memory_env, "TESTPATH_VIRTUAL_MEMORY=%s", testpath_virtual_memory);
    sprintf(testpath_physical_memory_env, "TESTPATH_PHYSICAL_MEMORY=%s", testpath_physical_memory);
    sprintf(real_time_limit_env, "REAL_TIME_LIMIT=%d", real_time_limit);
    sprintf(virtual_memory_limit_env, "VIRTUAL_MEMORY_LIMIT=%d", virtual_memory_limit);
    sprintf(testpath_stderr_env, "TESTPATH_STDERR=%s", testpath_error);

    int input_fd = open(testpath_input, O_RDONLY);

    if (input_fd == -1) {

        close(input_fd);
        if (DEBUG) printf("failed to open input file");
        return 1;

    }

    int output_fd = open(testpath_output, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    if (output_fd == -1) {

        close(output_fd);
        if (DEBUG) printf("failed to open output file");
        return 1;

    }

    int error_fd = open(testpath_error, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    if (error_fd == -1) {

        close(error_fd);
        if (DEBUG) printf("failed to error file");
        return 1;

    }

    char *envp[] = {

        language_env,
        testpath_time_env,
        testpath_cpu_time_env,
        testpath_virtual_memory_env,
        testpath_physical_memory_env,
        real_time_limit_env,
        virtual_memory_limit_env,
        testpath_stderr_env,
        NULL

    };

    pid_t child_pid = fork();

    if (child_pid == 0) {

        dup2(input_fd, STDIN_FILENO);
        dup2(output_fd, STDOUT_FILENO);
        dup2(error_fd, STDERR_FILENO);

        close(input_fd);
        close(output_fd);
        close(error_fd);

        execve("run", args, envp);

    } else if (child_pid > 0) {

        close(input_fd);
        close(output_fd);
        
        int status;
        waitpid(child_pid, &status, 0);

        int wexitstatus = WEXITSTATUS(status);

        printf("%d", WEXITSTATUS(status));

        if (wexitstatus == 0 || wexitstatus == 2 ||  wexitstatus == 4 || wexitstatus == 3) {

            int time, cpu_time, virtual_memory, physical_memory;

            FILE *file_time;
            FILE *file_cpu_time;
            FILE *file_virtual_memory;
            FILE *file_physical_memory;

            file_time = fopen(testpath_time, "r");
            
            if (file_time == NULL) {
                if(DEBUG) printf("file_time = NULL\n");
                return 1;
                
            }
            
            fscanf(file_time, "%d", &time);
            fclose(file_time);
            
            file_cpu_time = fopen(testpath_cpu_time, "r");
            
            if (file_cpu_time == NULL) {

                if(DEBUG) printf("file_cpu_time = NULL\n");
                return 1;
                
            }
            
            fscanf(file_cpu_time, "%d", &cpu_time);
            fclose(file_cpu_time);

            file_virtual_memory = fopen(testpath_virtual_memory, "r");

            if (file_virtual_memory == NULL) {

                if(DEBUG) printf("file_virtual_memory = NULL\n");
                return 1;
                
            }

            fscanf(file_virtual_memory, "%d", &virtual_memory);
            fclose(file_virtual_memory);

            file_physical_memory = fopen(testpath_physical_memory, "r");

            if (file_physical_memory == NULL) {

                if(DEBUG) printf("file_physical_memory = NULL\n");
                return 1;
                
            }

            fscanf(file_physical_memory, "%d", &physical_memory);
            fclose(file_physical_memory);

            result->status = wexitstatus;
            result->time = time;
            result->cpu_time = cpu_time;
            result->virtual_memory = virtual_memory;
            result->physical_memory = physical_memory;
            result->description = "";

            return 0;
        
        } else {

            result->status = 6;
            result->time = 0;
            result->cpu_time = 0;
            result->virtual_memory = 0;
            result->physical_memory = 0;
            return 1; //error

        }

    } else {

        if (DEBUG) printf("failed to create the first child process");
        result->status = 6;
        result->time = 0;
        result->cpu_time = 0;
        result->virtual_memory = 0;
        result->physical_memory = 0;
        return 1; //error
        
    }


}

struct TestResult *check_test_case(int submission_id, int test_case_id, char *language, char *input, char *solution, int real_time_limit, int virtual_memory_limit) {

    struct TestResult *result = malloc(sizeof(struct TestResult));
    result->status = 6; // define for error
    result->time = 0; 
    result->cpu_time = 0;
    result->virtual_memory = 0;
    result->physical_memory = 0;
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

    if (file_output == NULL) {

        if(DEBUG) printf("file_output = NULL\n");
        return result;

    }

    fclose(file_output);

    if (strcmp(language, "Python 3 (3.10)") == 0 || strcmp(language, "Node.js (20.x)") == 0) {

        bool py = strcmp(language, "Python 3 (3.10)") == 0;

        char code_path[path_length + getbytes(submission_id) + 4]; // path_length + getbytes(submission_id) + 4 (/.py)
        sprintf(code_path, py ? "checker_files/%d/%d.py" : "checker_files/%d/%d.js", submission_id, submission_id);

        char *file = py ? "/usr/bin/python3" : "/usr/bin/node";
        char *args[] = {file, code_path, NULL};

        int exec_status = execute(
            submission_id,
            test_case_id,
            args,
            result,
            real_time_limit,
            virtual_memory_limit,
            language);

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
                    submission_id,
                    test_case_id,
                    args,
                    result,
                    real_time_limit,
                    virtual_memory_limit,
                    language);

        if (exec_status == 1) {

            if (DEBUG) printf("failed in child process");
            return result;

        }

    } else {

        if (DEBUG) printf("unknown language");
        return result;

    }

    if (result->status == 3) {

        return result; //memory limit

    }

    if (result->status == 4) {

        return result; //runtime error

    }

    if(result->status == 2) {

        return result; //Time limit

    }

    file_output = fopen(testpath_output, "r");
    file_solution = fopen(testpath_solution, "r");

    char output_buffer[1000000], solution_buffer[1000000];
    
    char *output_read = fgets(output_buffer, 1000000, file_output);
    
    char *solution_read = fgets(solution_buffer, 1000000, file_solution);
    
    int status = -1; // checking 

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

    fclose(file_solution);      
    fclose(file_output);

    result->status = status;

    return result;
    
}

struct DebugResult {

    int status; /* 0 - Successful
                   2 - Time limit
                   3 - Memory limit
                   4 - Runtime error
                   5 - Compilation error
                   6 - server error */      
    int time;
    int cpu_time;
    int virtual_memory;
    int physical_memory;
    char *output;
    char *description;

};


struct DebugResult *debug(int debug_submission_id, int debug_test_id, char *language, char *input) {

    struct DebugResult *result = malloc(sizeof(struct DebugResult));
    struct TestResult *exec_result = malloc(sizeof(struct TestResult));

    result->status = 6; // define for error
    result->time = 0; 
    result->cpu_time = 0;
    result->virtual_memory = 0;
    result->physical_memory = 0;
    result->description = "";

    char output[1000000] = "";

    const int path_length = 14 + getbytes(debug_submission_id);
    const int testpath_input_length = path_length + getbytes(debug_test_id) + 11;

    char testpath_input[testpath_input_length]; //..._input.txt 
    char testpath_output[testpath_input_length + 1]; //..._output.txt

    sprintf(testpath_input, "checker_files/%d/%d_input.txt", debug_submission_id, debug_test_id);
    sprintf(testpath_output, "checker_files/%d/%d_output.txt", debug_submission_id, debug_test_id);

    FILE *file_output;
    FILE *file_input;

    file_input = fopen(testpath_input, "w");

    if (file_input == NULL) {

        if(DEBUG) printf("file_input = NULL\n");
        return result;
        
    }

    fprintf(file_input, "%s", input);
    fclose(file_input);

    file_output = fopen(testpath_output, "w");

    if (file_output == NULL) {

        if(DEBUG) printf("file_output = NULL\n");
        return result;

    }

    fclose(file_output);

    if (strcmp(language, "Python 3 (3.10)") == 0 || strcmp(language, "Node.js (20.x)") == 0) {

        bool py = strcmp(language, "Python 3 (3.10)") == 0;

        char code_path[path_length + getbytes(debug_submission_id) + 4]; // path_length + getbytes(debug_submission_id) + 4 (/.py)
        sprintf(code_path, py ? "checker_files/%d/%d.py" : "checker_files/%d/%d.js", debug_submission_id, debug_submission_id);

        char *file = py ? "/usr/bin/python3" : "/usr/bin/node";
        char *args[] = {file, code_path, NULL};

        int exec_status = execute(
            debug_submission_id,
            debug_test_id,
            args,
            exec_result,
            10,
            1024,
            language);

        if (exec_status == 1) {
            if (DEBUG) printf("failed in child process");
            return result;
        }
        
    } else if (strcmp(language, "C++ 17 (g++ 11.2)") == 0 || strcmp(language, "C 17 (gcc 11.2)") == 0) {

        int code_path_length = path_length + getbytes(debug_submission_id) + 1; // path_length + getbytes(debug_submission_id) + 1 :(/) :)))))))))))))))))))))))))))))))))

        char code_path[code_path_length]; 
        sprintf(code_path, "checker_files/%d/%d", debug_submission_id, debug_submission_id);

        char *file = code_path;
        char *args[] = {file, NULL};

        int exec_status = execute(
                    debug_submission_id,
                    debug_test_id,
                    args,
                    exec_result,
                    10,
                    1024,
                    language);

        if (exec_status == 1) {

            if (DEBUG) printf("failed in child process");
            return result;

        }

    } else {

        if (DEBUG) printf("unknown language");
        return result;

    }

    result->status = exec_result->status;

    file_output = fopen(testpath_output, "r");

    char output_buffer[1000000];

    while(fgets(output_buffer, 1000000, file_output)) {

        strcat(output, output_buffer);
        strcat(output, "\n");

    }

    strcat(output, "\0");
    
    result->output = output;
    result->time = exec_result->time;
    result->cpu_time = exec_result->cpu_time;
    result->virtual_memory = exec_result->virtual_memory;
    result->physical_memory = exec_result->physical_memory;
    
    if(result->status == 4) { //runtime error

        char error_buffer[1000];
        char error[10000];

        char testpath_error[testpath_input_length + 10]; //..._error.txt
        sprintf(testpath_error, "checker_files/%d/%d_stderr.txt", debug_submission_id, debug_test_id);

        FILE *ferr;
        ferr = fopen(testpath_error, "r");

        while (fgets(error_buffer, sizeof(error_buffer), ferr) != NULL) {

            strcat(error, error_buffer);
            strcat(error, "\n");

        }

        pclose(ferr);
        strcat(error, "\0");
        result->description = error;

        return result;
    }
    if (result->status == 2) {

        return result; //time limit

    }

    result->status = 0; //successful

    return result;

}
      
int main() {

    DEBUG = 1;

    // struct CreateFilesResult *cfr = create_files(12312365, "print(int(input()) ** 2)", "Python 3 (3.10)");
    // struct CreateFilesResult *cfr = create_files(12312365, "const Console = require(\"efrog\").Console;\nConsole.write(Number(Console.readAll()) ** 2);", "Node.js (20.x)");
    struct CreateFilesResult *cfr = create_files(12312365, "#include <iostream>\n\nusing namespace std;\nint main() {\nint* nptr = NULL;\ncout << *nptr;\n    int a;\n    cin >> a;\n    cout << a * a;\n    return 0;\n}", "C++ 17 (g++ 11.2)");
    //struct CreateFilesResult *cfr = create_files(12312365, "#include <stdio.h>\nint main () {\nint a;\nscanf(\"%d\", &a);\n}", "C 17 (gcc 11.2)");
    printf(
    "CreateFilesResult:\nstatus: %d\ndesctiption: %s\n", 
    cfr->status,
    cfr->description);

    struct TestResult *result = check_test_case(12312365, 12, "C++ 17 (g++ 11.2)", "12", "144", 4, 2);
    //struct DebugResult *result = debug(12312365, 12, "Python 3 (3.10)", "12");
    delete_files(12312365);

    printf(
        "TestCaseResult:\nstatus: %d\ntime: %dms\ncpu_time: %dms\nmemory: %dKB\nVM: %dKB", 
        result->status, 
        result->time, 
        result->cpu_time, 
        result->physical_memory,
        result->virtual_memory);
    
    // printf(
    //     "DebugResult:\nstatus: %d\ntime: %dms\ncpu_time: %dms\nmemory: %dKB\ndescription: %s\noutput: %s", 
    //     result->status, 
    //     result->time, 
    //     result->cpu_time, 
    //     result->memory,
    //     result->description,
    //     result->output);

    return 0;

}