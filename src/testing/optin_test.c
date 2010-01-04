#include <stdio.h>

#include "../include/optin.h"

int main(int argc, char** argv) {
    int ret;
    optin* o;
    
    /* Option variables */
    int test = -1;
    int ival1 = 10;
    int ival2 = 0;
    float fval1 = 0.7f;
    float fval2 = 0.0f;
    int flagval1 = 0;
    int flagval2 = 0;
    char* strval1 = "foo";
    char* strval2 = "";
            
    ret = -1;
    o = optin_new();
    if (!optin_has_option(o, "help"))  {
        fprintf(stderr, "ERROR: built-in option \"help\" is NOT present\n");
        goto done;
    }
    
    optin_add_int(o, "test", "The test number to run", OPTIN_REQUIRED, &test);
    optin_add_int(o, "ival1", "First integer value", OPTIN_HAS_DEFAULT, &ival1);
    optin_add_int(o, "ival2", "Second integer value", OPTIN_REQUIRED, &ival2);
    optin_add_float(o, "fval1", "First float value", OPTIN_HAS_DEFAULT, &fval1);
    optin_add_float(o, "fval2", "Second float value", OPTIN_REQUIRED, &fval2);
    optin_add_flag(o, "flagval1", "First flag", OPTIN_HAS_DEFAULT, &flagval1);
    optin_add_flag(o, "flagval2", "Second flag", OPTIN_REQUIRED, &flagval2);
    optin_add_string(o, "strval1", "First string value", OPTIN_HAS_DEFAULT, &strval1);
    optin_add_string(o, "strval2", "Second string value", OPTIN_REQUIRED, &strval2);
    
    if (optin_has_option(o, "xyzzy"))   {
        fprintf(stderr, "ERROR: optin_has_option() returned positive for test of non-existent option\n");
        goto done;
    }
    
    ret = optin_process(o, &argc, &argv);
    
    switch(test)    {
    default: 
        fprintf(stderr, "Invalid test number: %d\n", test);
        ret = -1;
        break;
    }
    
done:
    if (ret != 0)  {
        fprintf(stderr, "TEST FAILED, PRINTING DIAGNOSTIC INFORMATION:\n");
        optin_debug_print(o);
    }
    
    optin_destroy(o);
    return ret;
}