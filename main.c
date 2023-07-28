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

int DEBUG = 0;

int create_files(int submission_id, char *code, char *language) {

    const int path_length = 14 + (int)((floor(log10(submission_id)) + 2));
    char path[path_length]; 

    sprintf(path, "checker_files/%d", submission_id);
    int status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    if (status != 0) { //error

        if(DEBUG) printf("failed to create a dir\n");
        return 1;

    } 

    FILE  *file_code;
    
    if (strcmp(language, "Python 3 (3.10)") == 0) {

        char code_path[path_length + (int)((floor(log10(submission_id)) + 2)) + 4]; // path_length + (int)((floor(log10(submission_id)) + 2)) + 4 (/.py)
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
        int code_path_length = path_length + (int)((floor(log10(submission_id)) + 2)) + 5;
        char code_path[code_path_length]; 
        sprintf(code_path, cpp ? "%s/%d.cpp" : "%s/%d.c", path, submission_id);
        file_code = fopen(code_path, "w");

        if (file_code == NULL) {

            if(DEBUG) printf("file_code = NULL\n");
            return 1;

        }

        fprintf(file_code, "%s", code);
        fclose(file_code);

        char compile_command[11 +  2 * code_path_length]; //g++-11 main.cpp -o main len : 7 +  2 * code_path_length
        sprintf(compile_command, cpp ? "g++-11 %s -o %s/%d" : "gcc-11 %s -lm -o %s/%d", code_path, path, submission_id); 
        int status = system(compile_command); 
        if (status == 1) { 
            if (DEBUG) printf("failed to compile (cpp)\n");
        }

    } else {

        if (DEBUG) printf("unknown language\n");
        return 1;

    }

}

int delete_files(int submission_id) {
    const int path_length = 14 + (int)((floor(log10(submission_id)) + 2));

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
    int time;
    int memory;
    char *output;
    char *description;
};

struct Result *check_test_case(int submission_id, int test_case_id, char *language, char *input, char *solution) {

    struct Result *result = malloc(sizeof(struct Result));

    char output[1000000] = "";

    const int path_length = 14 + (int)((floor(log10(submission_id)) + 2));
    const int testpath_input_length = path_length + (int)((floor(log10(test_case_id)) + 2)) + 11;

    char testpath_input[testpath_input_length]; //..._input.txt
    char testpath_solution[testpath_input_length + 3]; //..._solution.txt

    sprintf(testpath_input, "checker_files/%d/%d_input.txt", submission_id, test_case_id);
    sprintf(testpath_solution, "checker_files/%d/%d_solution.txt", submission_id, test_case_id);

    FILE *file_output;
    FILE *file_input;
    FILE *file_solution;

    file_input = fopen(testpath_input, "w");

    if (file_input == NULL) {

        if(DEBUG) printf("file_input = NULL\n");
        result->status = 6;
        result->time = 0;
        result->memory = 0;
        result->output = "";
        result->description = "";
        return result;
        
    }

    fprintf(file_input, "%s", input);
    fclose(file_input);

    file_solution = fopen(testpath_solution, "w");

    if (file_solution == NULL) {

        if(DEBUG) printf("file_solution = NULL\n");
        result->status = 6;
        result->time = 0;
        result->memory = 0;
        result->output = "";
        result->description = "";
        return result;

    }

    fprintf(file_solution, "%s", solution);
    fclose(file_solution);

    
    if (strcmp(language, "Python 3 (3.10)") == 0) {

        int command_status;

        char code_path[path_length + (int)((floor(log10(submission_id)) + 2)) + 4]; // path_length + (int)((floor(log10(submission_id)) + 2)) + 4 (/.py)
        sprintf(code_path, "checker_files/%d/%d.py", submission_id, submission_id);
        
        char command[2 * path_length + testpath_input_length - 2]; 
        sprintf(command, "python3 %s < %s", code_path, testpath_input);

        file_output = popen(command, "r");

    } else if (strcmp(language, "C++ 17 (g++ 11.2)") == 0 || strcmp(language, "C 17 (gcc 11.2)") == 0) {

        int command_status;
        int code_path_length = path_length + (int)((floor(log10(submission_id)) + 2)) + 1; // path_length + (int)((floor(log10(submission_id)) + 2)) + 1 :(/) :)))))))))))))))))))))))))))))))))

        char code_path[code_path_length]; 
        sprintf(code_path, "checker_files/%d/%d", submission_id, submission_id);

        char command[code_path_length + testpath_input_length + 3]; 
        sprintf(command, "%s < %s", code_path, testpath_input);

        file_output = popen(command, "r");

    } else {

        if (DEBUG) printf("unknown language");
        result->status = 6;
        result->time = 0;
        result->memory = 0;
        result->output = "";
        result->description = "";
        return result;

    }

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
    result->time = 0;
    result->memory = 0;
    result->output = output;
    result->description = "";

    return result;
    
}
      
int main() {

    DEBUG = 1;

    create_files(12312365, "print(int(input()) ** 2)", "Python 3 (3.10)");
    struct Result *result = check_test_case(12312365, 123123, "Python 3 (3.10)", "20", "400");
    delete_files(12312365);

    printf("status: %d\noutput: %s", result->status, result->output);
    return 0;

}


