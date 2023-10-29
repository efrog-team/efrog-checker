struct CreateFilesResult {
    int status; /*
                0 - Successful
                5 - Compilation error
                6 - Server error 
                */
    char *description;
};

struct CreateFilesResult *create_files(int submission_id, char *code, char *language, int submission) // if submission = 1 S_id else D_id

int delete_files(int submission_id, int submission) // if submission = 1 S_id else D_id

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

struct TestResult *check_test_case(int submission_id, int test_case_id, char *language, char *input, char *solution, int real_time_limit, int physical_memory_limit, int submission) // if submission = 1 S_id else D_id