#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef int (*test_fn)(FILE* in, FILE* out, int argc, char** argv);

#define DEFINE_TEST_FUNCTION int test_function(FILE* in, FILE* out, int argc, char** argv)
#define RUN_TEST return test_main(test_function, argc, argv)

/**
 * Returns zero if two files are identical, non-zero otherwise
 */
int diff_files(const char* name1, const char* name2)    {
    char cmd[512];
    memset(cmd, 0, 512);
    sprintf(cmd, "diff %s %s", name1, name2);
    return system(cmd) != 0;    
}

int test_runner(char* infile, char* bmkfile, test_fn fn, int argc, char** argv) {
    FILE* in;
    FILE* out;
    int res;
    char tempname[255];
    
    if (!infile)    {
        in = stdin;
    } else {
        in = fopen(infile, "r");
        if (!in)    {
            fprintf(stderr, "Could not open %s for reading\n", infile);
            return -1;
        }
    }
    
    if (!bmkfile)   {
        out = stdout;
    } else {
        memset(tempname, 0, 255);
        tmpnam(tempname);
        out = fopen(tempname, "w");
        if (!out)   {
            fprintf(stderr, "Could not open %s for writing\n", tempname);
        }
    }
    
    res = fn(in, out, argc, argv);
    
    if (infile) {
        fclose(in);
    }
    
    if (bmkfile)    {
        fclose(out);
        
        if (res == 0)   {
            // Success from the actual test function, so pass it to the diff and see if the 
            // benchmark matches the output
            res = diff_files(bmkfile, tempname);
        }
        
        remove(tempname);
    }
    
    return res;
}

int test_main(test_fn fn, int argc, char** argv)    {
    char* infile;
    char* bmkfile;
    int res;
    
    if (argc > 4)   {
        fprintf(stderr, "USAGE: <testname>_test <input file> <benchmark file> <optional arg>\n");
        fprintf(stderr, "\t<testname>_test <input_file>\n");
        return 1;
    }
    
    if (argc > 1)   {
        infile = argv[1];
    } else {
        infile = 0;
    }
    
    if (argc > 2 && strcmp(argv[2], "0"))   {
        bmkfile = argv[2];
    } else { 
        bmkfile = 0;
    }
    
    return test_runner(infile, bmkfile, fn, argc, argv);
}

int get_file_length(const char* filename)   {
    int length;
    FILE* fd;
    
    fd = fopen(filename, "rb");
    if (!fd)    {
        return 0;
    }
    
    fseek(fd, 0, SEEK_END);
    length = ftell(fd);
    close(fd);
    
    return length;
}

#endif // TEST_UTILS_H