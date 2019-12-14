#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>

#define MAX_BUFFER 150
#define MAX_MESSAGE 256

typedef struct student {
    char name[MAX_BUFFER];
    int grade;
} student;

void handleConfigurationFile(char* configpath, char array[3][MAX_BUFFER]);
student* initialStudentArray(char* directory);
void scanSubDir(char *studentPath, int subfilesfd);
char *getCPath(int fd, char *path);
int isCFile(char* filename);
int isDirectory(char* path);
char* getPreviousPath(char* fullpath);
char* getFileName(char* fullpath);
void removeEndline(char* string);
int countStudents(char* path);
int compileCFile(char* path);
int executeCFile(char *path, int inputFile);
int compareCFile(char* path, char* correctOutput);
int gradeStudent(char *path, int inputfd, char *correctOutput);
void handleStudent(student *stud, char *directory, char *inputFile, char *outputFile, int inputfd);
void createCSV(student* arr, char* directory);

int main(int argc, char* argv[]) {
    char* configpath = argv[1];
    char configs[3][MAX_BUFFER];
    handleConfigurationFile(configpath, configs);
    // separate all the needed information from config file.
    char* directory = configs[0];
    char* inputFilePath = configs[1];
    char* outputFilePath = configs[2];
    // open input file.
    int inputfd = open(inputFilePath, O_RDONLY);
    if (inputfd < 0) {
        write(STDERR_FILENO, "Error in system call.\n", MAX_MESSAGE);
    }
    // init a student array.
    student* students = initialStudentArray(directory);
    // grade the students one by one.
    int i = 0;
    int studentsNum = countStudents(directory);
    while (i < studentsNum) {
        chdir(getenv("HOME"));
        handleStudent(&students[i], directory, inputFilePath, outputFilePath, inputfd);
        i++;
    }
    chdir(getenv("HOME"));
    // create a csv file with grades.
    createCSV(students, directory);
    free(students);
    // close the input file.
    close(inputfd);
}

/*
 * Separate the 3 directories from the configuration text file into 3 buffers.
 */
void handleConfigurationFile(char* configpath, char array[3][MAX_BUFFER]) {
    // open the configuration file and handle it.
    int configfd = open(configpath, O_RDONLY);
    if (configfd < 0) {
        write(STDERR_FILENO, "Error in system call.\n", MAX_MESSAGE);
        return;
    }
    int i = 0;
    int j = 0;
    // read the text from configuration file to temp buffer.
    char* buffer = "";
    struct stat v;
    stat(configpath, &v);
    size_t size = (size_t) v.st_size;
    buffer = malloc(size * sizeof(char));
    read(configfd, buffer, size);
    // add the first line of file to the array.
    while (buffer[i] != '\n') {
        array[0][j] = buffer[i];
        i++;
        j++;
    }
    array[0][j] = '\0';
    i++;
    j = 0;
    // add the second line of file to the array.
    while (buffer[i] != '\n') {
        array[1][j] = buffer[i];
        i++;
        j++;
    }
    array[1][j] = '\0';
    i++;
    j = 0;
    // add the third line of file to the array.
    while (buffer[i] != '\n') {
        array[2][j] = buffer[i];
        i++;
        j++;
    }
    array[2][j] = '\0';
}

/*
 * Create the array of students and initialize grades with 0.
 */
student* initialStudentArray(char* directory) {
    student* students = (student*) malloc(0);
    DIR* dp;
    struct dirent* ep;
    dp = opendir(directory);
    int i = 0;
    // scan all the subfolders of the directory and create a student struct for each student. grade will be given later.
    if (dp != NULL) {
        while (ep = readdir (dp)) {
            // ignore "." and "..".
            if ((strcmp(ep->d_name, ".") != 0) && (strcmp(ep->d_name, "..") != 0)) {
                students = (student*) realloc(students, (i + 1) * sizeof(student));
                student temp;
                strcpy(temp.name, ep->d_name);
                temp.grade = 0;
                students[i] = temp;
                i++;
            }
        }
        (void) closedir(dp);
    } else {
        perror("Couldn't open the directory.");
    }
    return students;
}

/*
 * Count the students by checking how many folders exist in the directory.
 */
int countStudents(char* path) {
    DIR* dp;
    struct dirent* ep;
    dp = opendir(path);
    int count = 0;
    // scan all the subfolders of the directory and count each folder.
    if (dp != NULL) {
        while (ep = readdir (dp)) {
            // ignore "." and "..".
            if ((strcmp(ep->d_name, ".") != 0) && (strcmp(ep->d_name, "..") != 0)) {
                count++;
            }
        }
        (void) closedir(dp);
    } else {
        perror("Couldn't open the directory.");
    }
    return count;
}

/*
 * Scan a specific student's directory (for finding the c file in it).
 * For each file - if it's a directory, recursively scan it.
 *                 if it's a file, add it to the sub-files list.
 */
void scanSubDir(char *studentPath, int subfilesfd) {
    DIR* dp;
    struct dirent* ep;
    // go to the specific student's folder.
    chdir(getenv("HOME"));
    dp = opendir(studentPath);
    // scan all the sub-directories of the student and look for files.
    if (dp != NULL) {
        while (ep = readdir (dp)) {
            if ((strcmp(ep->d_name, ".") == 0) || (strcmp(ep->d_name, "..") == 0)) {
                continue;
            }
            char* path = malloc(sizeof(studentPath) + sizeof(ep->d_name) + 1);
            strcpy(path, studentPath);
            strcat(path, "/");
            strcat(path, ep->d_name);
            // if path is a directory, recursively scan it.
            if(isDirectory(path) == 1) {
                scanSubDir(path, subfilesfd);
                free(path);
                // if path is a file, write it to the text file.
            } else {
                char* temp = malloc(300);
                sprintf(temp, "%s\n", path);
                write(subfilesfd, temp, strlen(temp));
                free(temp);
            }
        }
        (void) closedir(dp);
    } else {
        perror("Couldn't open the directory.");
    }
}

/*
 * Find the c file in the list of files that were found in the student's directory.
 */
char* getCPath(int fd, char* path) {
    strcat(path, "/subfiles.txt");
    int i = 0;
    // if the file is empty, return null immediately.
    int size = (int) lseek(fd, 0, SEEK_END);
    if (size == 0) {
        return NULL;
    }
    struct stat v;
    stat(path, &v);
    size_t buffsize = (size_t) v.st_size;
    char* buffer = (char *) malloc(buffsize * sizeof(char));
    lseek(fd, 0, SEEK_SET);
    read(fd, buffer, buffsize);
    // if not empty, rewind and start scanning line by line.
    int j = 0;
    char* temp = malloc(MAX_BUFFER * sizeof(char));
    for (i = 0; i < strlen(buffer); i++) {
        if (buffer[i] == '\n') {
            temp[j] = '\0';
            j++;
            if (isCFile(temp) == 1) {
                return temp;
            } else {
                temp[0] = '\0';
                j = 0;
            }
        } else {
            temp[j] = buffer[i];
            j++;
        }
    }
    return NULL;
}

/*
 * Check if a file name ends with ".c".
 */
int isCFile(char* filename) {
    // check if file ends with ".c".
    if ((filename[strlen(filename) - 2] == '.') && ((filename[strlen(filename) - 1] == 'c'))){
        return 1;
    }
    return 0;
}

/*
 * Check if a path is a directory or a file.
 */
int isDirectory(char* path) {
    // check if given path is a directory or a file.
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
}

/*
 * Remove the end-line char from a string.
 */
void removeEndline(char* string) {
    if((string[strlen(string) - 2] == '\r') && (string[strlen(string) - 1] == '\n')) {
        string[strlen(string) - 2] = '\0';
    }
    if(string[strlen(string) - 1] == '\r') {
        string[strlen(string) - 1] = '\0';
    }
    if(string[strlen(string) - 1] == '\n') {
        string[strlen(string) - 1] = '\0';
    }
}

/*
 * Find the previous path of a given path (one step backwards).
 */
char* getPreviousPath(char* fullpath) {
    char* previousPath = malloc(MAX_BUFFER * sizeof(char));
    strcpy(previousPath, fullpath);
    int i = (int) (strlen(previousPath) - 1);
    // start from the end and remove all the current folder from path.
    while (previousPath[i] != '/') {
        previousPath[i] = '\0';
        i--;
    }
    previousPath[i] = '\0';
    return previousPath;
}

/*
 * Find the file's name given its' full directory.
 */
char* getFileName(char* fullpath) {
    char* filename = malloc(MAX_BUFFER * sizeof(char));
    int i = (int) (strlen(fullpath) - 1);
    // start from the end and find where the file name begins.
    while (fullpath[i] != '/') {
        i--;
    }
    i++;
    int j = 0;
    // copy all the chars of file name one by one.
    while(fullpath[i] != '\0') {
        filename[j] = fullpath[i];
        i++;
        j++;
    }
    filename[j] = '\0';
    return filename;
}

/*
 * Compile the c file.
 */
int compileCFile(char* path) {
    char* compArgs[3] = {};
    removeEndline(path);
    // find the previous path
    char* previousPath = malloc(MAX_BUFFER * sizeof(char));
    strcpy(previousPath, getPreviousPath(path));
    // cd to previous folder.
    chdir(previousPath);
    // find the filename.
    char* filename = getFileName(path);
    // create an args array to pass to execvp.
    compArgs[0] = malloc(MAX_BUFFER * sizeof(char));
    strcpy(compArgs[0], "gcc");
    compArgs[1] = malloc(MAX_BUFFER * sizeof(char));
    strcpy(compArgs[1], filename);
    compArgs[2] = NULL;
    // fork the current process.
    pid_t pid = fork();
    // error forking the process.
    if (pid == -1) {
        write(STDERR_FILENO, "Error in system call.\n", MAX_MESSAGE);
        return -1;
    }
    // if pid = 0, it's the child's preocess
    else if (pid == 0) {
        // replace the child's process with the compilation command.
        if ((execvp(compArgs[0], compArgs) < 0)) {
            exit(1);
        }
        // if it's the father's proccess, wait for the child to end.
    } else {
        free(compArgs[0]);
        free(compArgs[1]);
        free(previousPath);
        int childpid;
        waitpid(pid, &childpid, 0);
        // if child process exited with 1, there was a compilation error.
        if (WIFEXITED(childpid)) {
            if(WEXITSTATUS(childpid) == 1) {
                return -1;
            }
        }
    }
    return 0;
}

/*
 * execute the c file.
 */
int executeCFile(char *path, int inputfd) {
    char* exeArgs[2] = {"./a.out", NULL};
    removeEndline(path);
    // find the previous path
    char* previousPath = getPreviousPath(path);
    // cd to previous folder.
    chdir(previousPath);
    // create a file for the output.
    int outputfd = open("output.txt",  O_CREAT | O_RDWR, 0666);
    if (outputfd < 0) {
        write(STDERR_FILENO, "Error in system call.\n", MAX_MESSAGE);
    }
    // fork the current process.
    pid_t pid = fork();
    // error forking the process.
    if (pid == -1) {
        write(STDERR_FILENO, "Error in system call.\n", MAX_MESSAGE);
        return -1;
    }
    // if pid = 0, it's the child's preocess
    else if (pid == 0) {
        // take the input from the input file.
        dup2(inputfd, STDIN_FILENO);
        // insert the output to the output file.
        dup2(outputfd, STDOUT_FILENO);
        // replace the child's process with the compilation command.
        if ((execvp(exeArgs[0], exeArgs) < 0)) {
            write(STDERR_FILENO, "Error in system call.\n", MAX_MESSAGE);
        }
        // if it's the father's proccess, wait for the child to end.
    } else {
        int childpid;
        sleep(5);
        if(!waitpid(pid, &childpid, WNOHANG)) {
            kill(pid, 0);
            return -1;
        }
    }
    return 0;
}

/*
 * Compare the c file to the correct output file.
 */
int compareCFile(char* path, char* correctOutput) {
    char* outputPath = malloc(sizeof(path) + MAX_BUFFER * sizeof(char));
    char* previousPath = getPreviousPath(path);
    strcpy(outputPath, previousPath);
    strcat(outputPath, "/output.txt");
    chdir(getenv("HOME"));
    char* args[4] = {"./comp.out", outputPath, correctOutput, NULL};
    // fork the current process.
    pid_t pid = fork();
    // error forking the process.
    if (pid == -1) {
        write(STDERR_FILENO, "Error in system call.\n", MAX_MESSAGE);
        return -1;
    }
        // if pid = 0, it's the child's preocess
    else if (pid == 0) {
        if ((execvp(args[0], args) < 0)) {
            write(STDERR_FILENO, "Error in system call.\n", MAX_MESSAGE);
        }
        // if it's the father's proccess, wait for the child to end.
    } else {
        int childpid;
        waitpid(pid, &childpid, 0);
        if (WIFEXITED(childpid)) {
            return WEXITSTATUS(childpid);
        }
    }
    return 0;
}

/*
 * Give the student a grade following the steps:
 * Check if there's a c file - if not, grade is 0 and stop.
 * Try compiling - if not compiling, grade is 20 and stop.
 * Execute the file - if the execution lasted for more than 5 seconds, it's a timeout. give 40 and stop.
 * If student passed all of the above, use comp.out of ex31 to compare his output with the correct output.
 * Bad output = 60, Similar output = 80, Identical output = 100.
 */
int gradeStudent(char *path, int inputfd, char *correctOutput) {
    // if there was no c file, return grade = 0.
    if (path == NULL) {
        return 0;
    }
    // compile the file. if there was a compilation error, return grade = 20.
    int comp = compileCFile(path);
    if (comp == -1) {
        return 20;
    }
    // run the student's program. if the program ran for more than 5 seconds, it's a time-out.
    int exe = executeCFile(path, inputfd);
    if (exe == -1) {
        return 40;
    }
    // if file compiled and did not time out, compare output with correct output and grade accordingly.
    int cmp = compareCFile(path, correctOutput);
    switch(cmp) {
        // identical.
        case 1:
            return 100;
        // different.
        case 2:
            return 60;
        // similar.
        case 3:
            return 80;
    }
}

/*
 * Prepare the student's path, create a subfile.txt and find the c file if exists. Then update the student's grade.
 */
void handleStudent(student *stud, char *directory, char *inputFile, char *outputFile, int inputfd) {
    // build a path to the specific student: directory/student_name.
    char* path = malloc(sizeof(stud->name) + sizeof(directory) + 1);
    sprintf(path, "%s/%s", directory, stud->name);
    chdir(path);
    // create a file with a list of all the files inside the student's directory.
    int subfilesfd = open("subfiles.txt", O_CREAT | O_RDWR, 0666);
    if (subfilesfd < 0) {
        write(STDERR_FILENO, "Error in system call.\n", MAX_MESSAGE);
        return;
    }
    // scan the student's folder and add all the files to subfiles.txt.
    scanSubDir(path, subfilesfd);
    char* cPath = getCPath(subfilesfd, path);
    // move pointer back to beginning of input file.
    lseek(inputfd, 0, SEEK_SET);
    // need to replace the print with grading..
    stud->grade = gradeStudent(cPath, inputfd, outputFile);
    // re-build the path to current student, change directory to it and remove all files.
    char* path2 = malloc(sizeof(stud->name) + sizeof(directory) + 1);
    sprintf(path2, "%s/%s", directory, stud->name);
    chdir(path2);
    unlink("subfiles.txt");
    unlink("output.txt");
    unlink("a.out");
    free(path);
    free(path2);
    free(cPath);
}

/*
 * Create a CSV file with all the grades.
 */
void createCSV(student* arr, char* directory) {
    int i = 0;
    int count = countStudents(directory);
    int resultsfd = open("results.csv", O_CREAT | O_RDWR | O_APPEND, 0666);
    if (resultsfd < 0) {
        write(STDERR_FILENO, "Error in system call.\n", MAX_MESSAGE);
        return;
    }
    for (i = 0; i < count; i++) {
        student current = arr[i];
        char *reason = malloc(25);
        // give a matching feedback.
        switch (current.grade) {
            case 0:
                strcpy(reason, "NO_C_FILE");
                break;
            case 20:
                strcpy(reason, "COMPILATION_ERROR");
                break;
            case 40:
                strcpy(reason, "TIMEOUT");
                break;
            case 60:
                strcpy(reason, "BAD_OUTPUT");
                break;
            case 80:
                strcpy(reason, "SIMILAR_OUTPUT");
                break;
            case 100:
                strcpy(reason, "GOOD_JOB");
                break;

        }
        char *line = malloc(500);
        sprintf(line, "%s,%d,%s\n", current.name, current.grade, reason);
        write(resultsfd, line, strlen(line));
        free(reason);
        free(line);
    }
}
