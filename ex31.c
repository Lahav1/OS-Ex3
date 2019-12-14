#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_MESSAGE 256
#define ASCII_DIFF 32

int checkIfIdentical(char* text1, char* text2);
int checkIfSimilar(char* text1, char* text2);
void removeWhitespace(char* text);

/*
 * 1 - Identical.
 * 2 - Different.
 * 3 - Similar.
 */
int main(int argc, char* argv[]) {
    // default case - files are different.
    int result = 2;
    char* path1 = argv[1];
    char* path2 = argv[2];
    int fd1;
    int fd2;
    char* text1;
    char* text2;
    // open files.
    fd1 = open(path1, O_RDONLY);
    if (fd1 < 0) {
        write(STDERR_FILENO, "Error in system call.\n", MAX_MESSAGE);
        return 0;
    } else {
        struct stat v;
        stat(path1, &v);
        size_t size = (size_t) v.st_size;
        text1 = malloc(size * sizeof(char));
        read(fd1, text1, size);
    }
    fd2 = open(path2, O_RDONLY);
    if (fd2 < 0) {
        write(STDERR_FILENO, "Error in system call.\n", MAX_MESSAGE);
        return 0;
    } else {
        struct stat v;
        stat(path2, &v);
        size_t size = (size_t) v.st_size;
        text2 = malloc(size * sizeof(char));
        read(fd2, text2, size);
    }
    // check first if files are identical.
    int isIdentical = checkIfIdentical(text1, text2);
    if (isIdentical == 1) {
        result = 1;
        // if files are not identical, check if similar.
    }
    else {
        removeWhitespace(text1);
        removeWhitespace(text2);
        int isSimilar = checkIfSimilar(text1, text2);
        if (isSimilar == 1) {
            result = 3;
        }
    }
//    switch (result) {
//        case 1:
//            printf("Identical");
//            break;
//        case 2:
//            printf("Different");
//            break;
//        case 3:
//            printf("Similar");
//            break;
//    }
    close(fd1);
    close(fd2);
    return result;
}

/*
 * Remove all the whitespaces from a text file.
 */
void removeWhitespace(char* text) {
    char* i = text;
    char* j = text;
    while(*j != '\0') {
        *i = *j;
        j++;
        if ((*i != ' ') && (*i != '\n')) {
            i++;
        }
    }
    *i = '\0';
}

/*
 * Check if 2 text files are identical.
 */
int checkIfIdentical(char* text1, char* text2) {
    int i1 = 0;
    int i2 = 0;
    // set isidentical to true.
    int isIdentical = 1;
    while (1) {
        // if one of the files reached end of file, stop the loop.
        if ((text1[i1] == '\0') || (text2[i2] == '\0')) {
            break;
        }
        if (text1[i1] != text2[i2]) {
            isIdentical = 0;
            break;
        }
        i1++;
        i2++;
    }
    // if only one index reached end of string the files are certainly not identical.
    if (((text1[i1] == '\0') && (text2[i2] != '\0')) || ((text1[i1] != '\0') && (text2[i2] == '\0'))) {
        isIdentical = 0;
    }
    return isIdentical;
}

/*
 * Check if 2 text files are similar (differ only in whitespaces and capital letters).
 */
int checkIfSimilar(char* text1, char* text2) {
    int i1 = 0;
    int i2 = 0;
    // set isSimilar to true.
    int isSimilar = 1;
    while (1) {
        if ((text1[i1] == '\0') || (text2[i2] == '\0')) {
            break;
        }
        // if the current chars are different and also not the same letter (capital and small), change to false.
        if ((text1[i1] != text2[i2]) && ((text1[i1] + ASCII_DIFF) != text2[i2]) &&
                            (text1[i1] != (text2[i2] + ASCII_DIFF))) {
            isSimilar = 0;
            break;
        }
        i1++;
        i2++;
    }
    if (((text1[i1] == '\0') && (text2[i2] != '\0')) || ((text1[i1] != '\0') && (text2[i2] == '\0'))) {
        isSimilar = 0;
    }
    return isSimilar;
}



