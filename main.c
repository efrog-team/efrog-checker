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

#define GiB (1024L * 1024L * 1024L)
#define MiB (1024L * 1024L)

#define MP_len 1000

#define cpp "C++ 17 (g++ 11.2)"
#define cs "C# (Mono 6.8)"
#define python "Python 3 (3.10)"
#define js "Node.js (20.x)"
#define c "C 17 (gcc 11.2)"

struct CreateFilesResult {
    int status; /*
                0 - Successful
                5 - Compilation error
                6 - Server error 
                */
    char *description;
};


struct CreateFilesResult *create_files(int submission_id, char *code, char *language, int submission) { // if submission = 1 S_id else D_id

    struct CreateFilesResult *result = malloc(sizeof(struct CreateFilesResult));

    result->status = 6; //define for server error
    result->description = "";

    char* cf_id_path = (char*)malloc(MP_len); //checker files path

    char* user_code_path = (char*)malloc(MP_len); //user code path in checker_files dir

    sprintf(cf_id_path, submission ? "checker_files/S_%d" : "checker_files/D_%d", submission_id);

    if (mkdir(cf_id_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) { //error: failed to create a dir

        printf("ERROR : Failed to create a dir 1\n");
        return result;

    } 

    char* solution_dir = (char*)malloc(MP_len); //...solution/id_solution.txt
    sprintf(solution_dir, "%s/solution", cf_id_path);

    if (mkdir(solution_dir, S_IRWXU) != 0) { //error: failed to create a dir

        printf("ERROR : Failed to create a dir 3\n");
        return result;

    } 

    char* cf_id_folder_path = (char*)malloc(MP_len);
    sprintf(cf_id_folder_path, "%s/program", cf_id_path);

    if (mkdir(cf_id_folder_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) { //error: failed to create a dir

        printf("ERROR : Failed to create a dir 1\n");
        return result;

    } 

    FILE  *user_code_file;

    /*---------------------------------------------------*/
    if (strcmp(language, python) == 0) { //python

        sprintf(user_code_path, "%s/main.py", cf_id_folder_path);

        user_code_file = fopen(user_code_path, "w");
        if (user_code_file == NULL) { // ERROR : user_code_file is not created

            printf("ERROR : user_code_file is not created\n");
            return result;

        }
        fprintf(user_code_file, "%s", code);
        fclose(user_code_file);
    /*---------------------------------------------------*/
    } else if (strcmp(language, js) == 0) { //js

        sprintf(user_code_path, "%s/index.js", cf_id_folder_path);

        user_code_file = fopen(user_code_path, "w");
        if (user_code_file == NULL) { // ERROR : user_code_file is not created

            printf("ERROR : user_code_file is not created\n");
            return result;

        }
        fprintf(user_code_file, "%s", code);
        fclose(user_code_file);
    /*---------------------------------------------------*/
    } else if (strcmp(language, cpp) == 0) { //cpp

        sprintf(user_code_path, "%s/main.cpp", cf_id_folder_path);

        user_code_file = fopen(user_code_path, "w");
        if (user_code_file == NULL) { // ERROR : user_code_file is not created

            printf("ERROR : user_code_file is not created\n");
            return result;

        }
        fprintf(user_code_file, "%s", code);
        fclose(user_code_file);

        //compilation

        char *compile_command = (char*)malloc(MP_len * 2);

        sprintf(compile_command, "g++-11 -static -s %s 2>&1 -o %s/main", user_code_path, cf_id_path);

        FILE *ferr = popen(compile_command, "r");

        char cerror_buffer[MP_len];
        char cerror[MP_len * 10];

        while (fgets(cerror_buffer, sizeof(cerror_buffer), ferr) != NULL) {

            strcat(cerror, cerror_buffer);
            strcat(cerror, "\n");

        }

        strcat(cerror, "\0");

        if (WEXITSTATUS(pclose(ferr)) != 0) { //ERROR : failed to compile C++

            printf("ERROR : failed to compile C++\n"); 

            result->status = 5;
            result->description = cerror;

            return result;
        }
    /*---------------------------------------------------*/
    } else if (strcmp(language, c) == 0) { //c

        sprintf(user_code_path, "%s/main.c", cf_id_folder_path);

        user_code_file = fopen(user_code_path, "w");
        if (user_code_file == NULL) { // ERROR : user_code_file is not created

            printf("ERROR : user_code_file is not created\n");
            return result;

        }
        fprintf(user_code_file, "%s", code);
        fclose(user_code_file);

        //compilation

        char *compile_command = (char*)malloc(MP_len * 2);

        sprintf(compile_command, "gcc-11 -static -s %s 2>&1 -o %s/main", user_code_path, cf_id_path);

        FILE *ferr = popen(compile_command, "r");

        char cerror_buffer[MP_len];
        char cerror[MP_len * 10];

        while (fgets(cerror_buffer, sizeof(cerror_buffer), ferr) != NULL) {

            strcat(cerror, cerror_buffer);
            strcat(cerror, "\n");

        }

        strcat(cerror, "\0");

        if (WEXITSTATUS(pclose(ferr)) != 0) { //ERROR : failed to compile C

            printf("ERROR : failed to compile C\n"); 

            result->status = 5;
            result->description = cerror;

            return result;
        }
    /*---------------------------------------------------*/
    } else if (strcmp(language, cs) == 0) { //cs

        sprintf(user_code_path, "%s/Program.cs", cf_id_folder_path);

        user_code_file = fopen(user_code_path, "w");
        if (user_code_file == NULL) { // ERROR : user_code_file is not created

            printf("ERROR : user_code_file is not created\n");
            return result;

        }
        fprintf(user_code_file, "%s", code);
        fclose(user_code_file);

        //compilation

        char *compile_command = (char*)malloc(MP_len * 2);

        sprintf(compile_command, "mcs %s 2>&1", user_code_path);

        FILE *ferr = popen(compile_command, "r");

        char cerror_buffer[MP_len];
        char cerror[MP_len * 10];

        while (fgets(cerror_buffer, sizeof(cerror_buffer), ferr) != NULL) {

            strcat(cerror, cerror_buffer);
            strcat(cerror, "\n");

        }

        strcat(cerror, "\0");

        if (WEXITSTATUS(pclose(ferr)) != 0) { //ERROR : failed to compile cs

            printf("ERROR : failed to compile cs\n"); 

            result->status = 5;
            result->description = cerror;

            return result;
        }
    /*---------------------------------------------------*/
    } else { // error: Unknown language

        printf("ERROR : Unknown language\n");
        return result;

    }

    result->status = 0;
    return result;
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
    int real_time_limit, //in seconds
    char *language,
    int submission) { 

    //define for error:

    result->status = 6;
    result->time = 0;
    result->cpu_time = 0;
    result->virtual_memory = 0;
    result->physical_memory = 0;

    char *testpath_input = (char*)malloc(MP_len); //..._input.txt
    char *testpath_output = (char*)malloc(MP_len); //..._output.txt
    char *testpath_error = (char*)malloc(MP_len); //..._error.txt
    char *testpath_time = (char*)malloc(MP_len); //..._time.txt
    char *testpath_cpu_time = (char*)malloc(MP_len); //..._cpu_time.txt
    //char *testpath_virtual_memory = (char*)malloc(MP_len); //..._virtual_memory.txt
    char *testpath_physical_memory = (char*)malloc(MP_len); //..._physical_memory.txt

    char* cf_id_path = (char*)malloc(MP_len); //checker files path
    sprintf(cf_id_path, submission ? "checker_files/S_%d" : "checker_files/D_%d", submission_id);

    char* root_dir = (char*)malloc(MP_len);//root dir
    sprintf(root_dir, "%s/program", cf_id_path); //root dir

    sprintf(testpath_input, "%s/%d_input.txt",  cf_id_path, test_case_id);
    sprintf(testpath_output, "%s/%d_output.txt", cf_id_path, test_case_id);
    sprintf(testpath_error, "%s/%d_stderr.txt", cf_id_path, test_case_id);
    sprintf(testpath_time, "%s/%d_time.txt",  cf_id_path, test_case_id);
    sprintf(testpath_cpu_time, "%s/%d_cpu_time.txt", cf_id_path, test_case_id);
    //sprintf(testpath_virtual_memory, "checker_files/%d/%d_virtual_memory.txt", submission_id, test_case_id);
    sprintf(testpath_physical_memory, "%s/%d_physical_memory.txt", cf_id_path, test_case_id);

    char *language_env = (char*)malloc(MP_len);
    char *testpath_time_env = (char*)malloc(MP_len);
    char *testpath_cpu_time_env = (char*)malloc(MP_len);
    //char *testpath_virtual_memory_env = (char*)malloc(MP_len);
    char *testpath_physical_memory_env = (char*)malloc(MP_len);

    char *real_time_limit_env = (char*)malloc(MP_len);
    //char virtual_memory_limit_env[21 + getbytes(virtual_memory_limit)]; 
    char *testpath_stderr_env = (char*)malloc(MP_len);
    char *root_dir_env = (char*)malloc(MP_len);

    sprintf(language_env, "LANGUAGE=%s", language);
    sprintf(testpath_time_env, "TESTPATH_TIME=%s", testpath_time);
    sprintf(testpath_cpu_time_env, "TESTPATH_CPU_TIME=%s", testpath_cpu_time);
    //sprintf(testpath_virtual_memory_env, "TESTPATH_VIRTUAL_MEMORY=%s", testpath_virtual_memory);
    sprintf(testpath_physical_memory_env, "TESTPATH_PHYSICAL_MEMORY=%s", testpath_physical_memory);
    sprintf(real_time_limit_env, "REAL_TIME_LIMIT=%d", real_time_limit);
    //sprintf(virtual_memory_limit_env, "VIRTUAL_MEMORY_LIMIT=%d", virtual_memory_limit);
    sprintf(testpath_stderr_env, "TESTPATH_STDERR=%s", testpath_error);
    sprintf(root_dir_env, "ROOT_DIR=%s", root_dir);

    int input_fd = open(testpath_input, O_RDONLY); 

    if (input_fd == -1) {//ERROR : failed to open input file

        close(input_fd);
        printf("ERROR : failed to open input file\n");
        return 1;

    }

    int output_fd = open(testpath_output, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    if (output_fd == -1) { //ERROR : failed to open output file

        close(output_fd);
        printf("ERROR : failed to open output file\n");
        return 1;

    }

    int error_fd = open(testpath_error, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    if (error_fd == -1) { // ERROR : failed to error file

        close(error_fd);
        printf("ERROR : failed to error file\n");
        return 1;

    }

    char *envp[] = {

        language_env,
        testpath_time_env,
        testpath_cpu_time_env,
        testpath_physical_memory_env,
        real_time_limit_env,
        testpath_stderr_env,
        root_dir_env,
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

        if (wexitstatus == 0 || wexitstatus == 2 ||  wexitstatus == 4 || wexitstatus == 3) {

            int time, cpu_time, physical_memory;
            //int virtual_memory;

            FILE *file_time;
            FILE *file_cpu_time;
            //FILE *file_virtual_memory;
            FILE *file_physical_memory;

            file_time = fopen(testpath_time, "r");
            
            if (file_time == NULL) { //ERROR : No file_time
                printf("ERROR : No file_time\n");
                return 1;
                
            }
            
            fscanf(file_time, "%d", &time);
            fclose(file_time);
            
            file_cpu_time = fopen(testpath_cpu_time, "r");
            
            if (file_cpu_time == NULL) { //ERROR : No file_cpu_time

                printf("ERROR : No file_cpu_time\n");
                return 1;
                
            }
            
            fscanf(file_cpu_time, "%d", &cpu_time);
            fclose(file_cpu_time);

            /*file_virtual_memory = fopen(testpath_virtual_memory, "r");

            if (file_virtual_memory == NULL) {

                if(DEBUG) printf("file_virtual_memory = NULL\n");
                return 1;
                
            }

            fscanf(file_virtual_memory, "%d", &virtual_memory);
            fclose(file_virtual_memory);*/

            file_physical_memory = fopen(testpath_physical_memory, "r");

            if (file_physical_memory == NULL) { //ERROR : No file_physical_memory

                printf("ERROR : No file_physical_memory\n");
                return 1;
                
            }

            fscanf(file_physical_memory, "%d", &physical_memory);
            fclose(file_physical_memory);

            result->status = wexitstatus;
            result->time = time;
            result->cpu_time = cpu_time;
            result->virtual_memory = 0;
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

        printf("ERROR : Failed to create the first child process (run)\n");
        result->status = 6;
        result->time = 0;
        result->cpu_time = 0;
        result->virtual_memory = 0;
        result->physical_memory = 0;
        return 1; //error
        
    }
}

struct TestResult *check_test_case(int submission_id, int test_case_id, char *language, char *input, char *solution, int real_time_limit, int physical_memory_limit, int submission) { // if submission = 1 S_id else D_id

    size_t max_output_size = 10 * MiB * sizeof(char);

    struct TestResult *result = malloc(sizeof(struct TestResult));

    result->status = 6; // define for error
    result->time = 0; 
    result->cpu_time = 0;
    result->virtual_memory = 0;
    result->physical_memory = 0;
    result->description = "";

    char* output = (char*)malloc(max_output_size);
    char* cf_id_path = (char*)malloc(MP_len); //checker files path

    sprintf(cf_id_path, submission ? "checker_files/S_%d" : "checker_files/D_%d", submission_id);

    char* testpath_input = (char*)malloc(MP_len); //..._input.txt 
    char* testpath_output = (char*)malloc(MP_len); //..._output.txt
    char* testpath_solution = (char*)malloc(MP_len); //..._solution.txt

    char* solution_dir = (char*)malloc(MP_len); //...solution/id_solution.txt
    sprintf(solution_dir, "%s/solution", cf_id_path);

    sprintf(testpath_input, "%s/%d_input.txt", cf_id_path, test_case_id);
    sprintf(testpath_output, "%s/%d_output.txt", cf_id_path, test_case_id);
    sprintf(testpath_solution, "%s/%d_solution.txt", solution_dir, test_case_id);

    FILE *file_output;
    FILE *file_input;
    FILE *file_solution;

    file_input = fopen(testpath_input, "w");

    if (file_input == NULL) { //Failed to create file_input

        printf("ERROR : Failed to create file_input\n"); 
        return result;
        
    }

    fprintf(file_input, "%s", input);
    fclose(file_input);

    file_solution = fopen(testpath_solution, "w"); 

    if (file_solution == NULL) {//Failed to create file_solution

        printf("ERROR : Failed to create file_solution\n");
        return result;

    }

    fprintf(file_solution, "%s", solution);
    fclose(file_solution);

    file_output = fopen(testpath_output, "w");

    if (file_output == NULL) { //Failed to create file_output

        printf("ERROR : Failed to create file_output\n");
        return result;

    }

    fclose(file_output);

    char* user_code_path = (char*)malloc(MP_len); //user code path in checker_files dir
    int exec_status;

    char* cf_id_folder_path = (char*)malloc(MP_len);
    sprintf(cf_id_folder_path, "%s/program", cf_id_path);

    if (strcmp(language, python) == 0) { //python

        sprintf(user_code_path, "%s/main.py", cf_id_folder_path);

        char *file = "/usr/bin/python3";
        char *args[] = {file, user_code_path, NULL};

        exec_status = execute(
            submission_id,
            test_case_id,
            args,
            result,
            real_time_limit,
            language,
            submission);

    /*---------------------------------------------------*/
    } else if (strcmp(language, js) == 0) { //js

        sprintf(user_code_path, "%s/index.js", cf_id_folder_path);
        char *file = "/usr/bin/node";
        char *args[] = {file, user_code_path, NULL};

        exec_status = execute(
            submission_id,
            test_case_id,
            args,
            result,
            real_time_limit,
            language,
            submission);

    /*---------------------------------------------------*/
    } else if (strcmp(language, cpp) == 0) { //cpp

        sprintf(user_code_path, "%s/main", cf_id_folder_path);

        char *file = user_code_path;
        char *args[] = {file, NULL};

        exec_status = execute(
            submission_id,
            test_case_id,
            args,
            result,
            real_time_limit,
            language,
            submission);
    /*---------------------------------------------------*/
    } else if (strcmp(language, c) == 0) { //c

        sprintf(user_code_path, "%s/main", cf_id_folder_path);

        char *file = user_code_path;
        char *args[] = {file, NULL};

        exec_status = execute(
            submission_id,
            test_case_id,
            args,
            result,
            real_time_limit,
            language,
            submission);

    /*---------------------------------------------------*/
    } else if (strcmp(language, cs) == 0) { //cs

        sprintf(user_code_path, "%s/Program.exe", cf_id_folder_path); // Mono .cs -> .exe compilation

        char *file = "/usr/bin/mono";
        char *args[] = {file, user_code_path, NULL};

        exec_status = execute(
            submission_id,
            test_case_id,
            args,
            result,
            real_time_limit,
            language,
            submission);

    /*---------------------------------------------------*/
    } else { // error: Unknown language

        printf("ERROR : Unknown language\n");
        return result;

    }

    if (exec_status == 1) { //Failed in child process

        printf("ERROR : Failed in child process");
        return result;

    }

    if (result->physical_memory > physical_memory_limit * 1024) { //if RSS usage > RSS limit (Postfactum) (comparison KiB and MiB) -> (KiB and KiB * 1024)

        result->physical_memory = physical_memory_limit * 1024;
        result->status = 3;

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

    char* output_buffer = (char*)malloc(max_output_size);
    char* solution_buffer = (char*)malloc(max_output_size);

    char *output_read = fgets(output_buffer, max_output_size, file_output);
    char *solution_read = fgets(solution_buffer, max_output_size, file_solution);
    
    int status = -1; // checking 

    while (output_read != NULL && solution_read != NULL) {

        for (int i = strlen(output_buffer) - 1; i >= 0; i--) {

            if (output_buffer[i] != '\n' && output_buffer[i] != ' ') { 

                output_buffer[i + 1] = '\0';
                break;
                
            }

        }

        for (int i = strlen(solution_buffer) - 1; i >= 0; i--) {

            if (solution_buffer[i] != '\n' && solution_buffer[i] != ' ' && solution_buffer[i] != '\r') { 

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

        output_read = fgets(output_buffer, max_output_size, file_output);
        solution_read = fgets(solution_buffer, max_output_size, file_solution);

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

int delete_files(int submission_id, int submission) { // if submission = 1 S_id else D_id
    
    char* cf_id_path = (char*)malloc(MP_len); //checker files path

    sprintf(cf_id_path, submission ? "checker_files/S_%d" : "checker_files/D_%d", submission_id);

    char* command = (char*)malloc(MP_len); //command to delete dir
    sprintf(command, "rm -rf %s", cf_id_path);

    if (system(command) == 0) {

        return 0;

    } else {

        printf("ERROR : failed to remove the dir");
        return 1;

    }
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

struct DebugResult *debug(int debug_submission_id, int debug_test_id, char *language, char *input, int submission) {

    //int output_max_size = 1024 * 1024;
    size_t max_output_size = 10 * MiB * sizeof(char); //10 MiB

    struct DebugResult *result = malloc(sizeof(struct DebugResult));
    struct TestResult *exec_result = malloc(sizeof(struct TestResult));

    result->status = 6; // define for error
    result->time = 0; 
    result->cpu_time = 0;
    result->virtual_memory = 0;
    result->physical_memory = 0;
    result->description = "";

    //char output[2000000] = "";
    char* output = (char*)malloc(max_output_size);

    char* cf_id_path = (char*)malloc(MP_len); //checker files path
    sprintf(cf_id_path, submission ? "checker_files/S_%d" : "checker_files/D_%d", debug_submission_id);

    char* testpath_input = (char*)malloc(MP_len);
    char *testpath_output = (char*)malloc(MP_len);

    sprintf(testpath_input, "%s/%d_input.txt", cf_id_path, debug_test_id);
    sprintf(testpath_output, "%s/%d_output.txt", cf_id_path, debug_test_id);

    FILE *file_output;
    FILE *file_input;

    file_input = fopen(testpath_input, "w");

    if (file_input == NULL) {

        printf("file_input = NULL\n");
        return result;
        
    }

    fprintf(file_input, "%s", input);
    fclose(file_input);

    file_output = fopen(testpath_output, "w");

    if (file_output == NULL) {

        printf("file_output = NULL\n");
        return result;

    }

    fclose(file_output);

    char* user_code_path = (char*)malloc(MP_len); //user code path in checker_files dir
    int exec_status;

    char* cf_id_folder_path = (char*)malloc(MP_len);
    sprintf(cf_id_folder_path, "%s/program", cf_id_path);

    if (strcmp(language, python) == 0) { //python

        sprintf(user_code_path, "%s/main.py", cf_id_folder_path);

        char *file = "/usr/bin/python3";
        char *args[] = {file, user_code_path, NULL};

        exec_status = execute(
            debug_submission_id,
            debug_test_id,
            args,
            exec_result,
            10,
            language,
            submission);

    /*---------------------------------------------------*/
    } else if (strcmp(language, js) == 0) { //js

        sprintf(user_code_path, "%s/index.js", cf_id_folder_path);
        char *file = "/usr/bin/node";
        char *args[] = {file, user_code_path, NULL};

        exec_status = execute(
            debug_submission_id,
            debug_test_id,
            args,
            exec_result,
            10,
            language,
            submission);

    /*---------------------------------------------------*/
    } else if (strcmp(language, cpp) == 0) { //cpp

        sprintf(user_code_path, "%s/main", cf_id_folder_path);

        char *file = user_code_path;
        char *args[] = {file, NULL};

        exec_status = execute(
            debug_submission_id,
            debug_test_id,
            args,
            exec_result,
            10,
            language,
            submission);
    /*---------------------------------------------------*/
    } else if (strcmp(language, c) == 0) { //c

        sprintf(user_code_path, "%s/main", cf_id_folder_path);

        char *file = user_code_path;
        char *args[] = {file, NULL};

        exec_status = execute(
            debug_submission_id,
            debug_test_id,
            args,
            exec_result,
            10,
            language,
            submission);

    /*---------------------------------------------------*/
    } else if (strcmp(language, cs) == 0) { //cs

        sprintf(user_code_path, "%s/Program.exe", cf_id_folder_path); // Mono .cs -> .exe compilation

        char *file = "/usr/bin/mono";
        char *args[] = {file, user_code_path, NULL};

        exec_status = execute(
            debug_submission_id,
            debug_test_id,
            args,
            exec_result,
            10,
            language,
            submission);

    /*---------------------------------------------------*/
    } else { // error: Unknown language

        printf("ERROR : Unknown language\n");
        return result;

    }

    if (exec_status == 1) { //Failed in child process

        printf("ERROR : Failed in child process");
        return result;

    }

        result->status = exec_result->status;


    file_output = fopen(testpath_output, "r");

    //char output_buffer[2000000]; 
    char* output_buffer = (char*)malloc(max_output_size);

    while(fgets(output_buffer, max_output_size, file_output)) {

        strcat(output, output_buffer);
        //strcat(output, "\n");

    }

    strcat(output, "\0");
    
    result->output = output;
    result->time = exec_result->time;
    result->cpu_time = exec_result->cpu_time;
    result->virtual_memory = 0;
    result->physical_memory = exec_result->physical_memory;
    
    if(result->status == 4) { //runtime error

        char error_buffer[1000];
        char error[10000];

        char testpath_error[MP_len]; //..._error.txt
        sprintf(testpath_error, "%s/%d_stderr.txt", cf_id_path, debug_test_id);

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


    if (result->physical_memory > 1024 * 1024) { //if RSS usage > RSS limit (Postfactum) (comparison KiB and MiB) -> (KiB and KiB * 1024)

        result->physical_memory = 1024 * 1024;
        result->status = 3;

    }

    if (result->status == 2) {

        return result; //time limit

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

    result->status = 0; //successful
    return result;

}

int main () {
    //delete_files(12312365, 1);
    // struct CreateFilesResult *cfr = create_files(12312365, "#include <iostream>\n\nusing namespace std;\nint main() {\nint t;\ncin >> t;\nfor(int i = 0; i < t; i++) {\nint a;\ncin >> a;\ncout << a * a << endl;\n}\nreturn 0;\n}", cpp, 1);
    struct CreateFilesResult *cfr = create_files(12312365, "console.log(12)", js, 1);
    // struct CreateFilesResult *cfr = create_files(12312365, "#include <stdio.h>\nint main () {\nint a;\nscanf(\"%d\", &a);\nprintf(\"%d\", a * a);\n}", "C 17 (gcc 11.2)", 1);
    // printf(
    // "CreateFilesResult:\nstatus: %d\ndesctiption: %s\n", 
    // cfr->status,
    // cfr->description);
    // struct TestResult *result = check_test_case(12312365, 12, c, "12", "144", 1, 90, 1);


    struct TestResult *result = check_test_case(12312365, 12, js, "12", "12", 1, 100, 1);

    printf(
    "TestCaseResult:\nstatus: %d\ntime: %dms\ncpu_time: %dms\nmemory: %dKB\n", 
    result->status, 
    result->time, 
    result->cpu_time, 
    result->physical_memory
    );
    // printf(
    //     "DebugResult:\nstatus: %d\ntime: %dms\ncpu_time: %dms\nmemory: %dKB\ndescription: %s\noutput: %s", 
    //     result->status, 
    //     result->time, 
    //     result->cpu_time, 
    //     result->physical_memory,
    //     result->description,
    //     result->output);

    
}

