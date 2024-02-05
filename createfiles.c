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

        printf("ERROR : Failed to create a dir\n");
        return result;

    } 

    FILE  *user_code_file;

    /*---------------------------------------------------*/
    if (strcmp(language, python) == 0) { //python

        sprintf(user_code_path, "%s/main.py", cf_id_path);

        user_code_file = fopen(user_code_path, "w");
        if (user_code_file == NULL) { // ERROR : user_code_file is not created

            printf("ERROR : user_code_file is not created\n");
            return result;

        }
        fprintf(user_code_file, "%s", code);
        fclose(user_code_file);
    /*---------------------------------------------------*/
    } else if (strcmp(language, js) == 0) { //js

        sprintf(user_code_path, "%s/index.js", cf_id_path);

        user_code_file = fopen(user_code_path, "w");
        if (user_code_file == NULL) { // ERROR : user_code_file is not created

            printf("ERROR : user_code_file is not created\n");
            return result;

        }
        fprintf(user_code_file, "%s", code);
        fclose(user_code_file);
    /*---------------------------------------------------*/
    } else if (strcmp(language, cpp) == 0) { //cpp

        sprintf(user_code_path, "%s/main.cpp", cf_id_path);

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

        sprintf(user_code_path, "%s/main.c", cf_id_path);

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

        sprintf(user_code_path, "%s/Program.cs", cf_id_path);

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

int main () {

    //struct CreateFilesResult *cfr = create_files(12312365, "using System;\nclass Program\n{\n    static void Main()\n    {\n        string input = Console.ReadLine();\n        double number = Convert.ToDouble(input);\n        double square = number * number;\n        Console.WriteLine($\"{square}\");\n    }\n}", "C# (Mono 6.8)", 1);
    // struct CreateFilesResult *cfr = create_files(12312365, "const Console = require(\"efrog\").Console;\nConsole.write(Number(Console.readAll()) ** 2);", "Node.js (20.x)", 1);
    // struct CreateFilesResult *cfr = create_files(12312365, "#include <iostream>\n\nusing namespace std;\nint main() {\nint t;\ncin >> t;\nfor(int i = 0; i < t; i++) {\nint a;\ncin >> a;\ncout << a * a << endl;\n}\nreturn 0;\n}", "C++ 17 (g++ 11.2)", 1);
    // struct CreateFilesResult *cfr = create_files(12312365, "#iclude <stdio.h>\nint main () {\nint a;\nscnf(\"%d\", &a);\n}", "C 17 (gcc 11.2)", 1);
    // struct CreateFilesResult *cfr = create_files(12312365, "print(int(input()) ** 2)", "Python 3 (3.10)", 1);
    // printf(
    // "CreateFilesResult:\nstatus: %d\ndesctiption: %s\n", 
    // cfr->status,
    // cfr->description);

}