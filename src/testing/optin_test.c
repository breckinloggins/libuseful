#include <stdio.h>

#include "optin.h"

int main(int argc, char** argv) {
    int ret, i;
    optin* o;
    
    /* Option variables */
    int test = -1;
    int ival1 = 10;
    int ival2 = 0;
    float fval1 = 0.7f;
    float fval2 = 0.0f;
    int flagval1 = 0;
    int flagval2 = 0;
    char* strval1;
    char* strval2;
    int i1 = 0;
    int i2 = 0;
    int i3 = 0;
            
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
    optin_add_flag(o, "flagval1", "First flag", OPTIN_REQUIRED, &flagval1);
    optin_add_flag(o, "flagval2", "Second flag", OPTIN_HAS_DEFAULT, &flagval2);
    optin_set_shortname(o, "flagval1", 'g');
    optin_add_string(o, "strval1", "First string value", OPTIN_HAS_DEFAULT, &strval1);
    optin_add_string(o, "strval2", "Second string value", OPTIN_REQUIRED, &strval2);
    optin_add_switch(o, "s1", "s1");
    optin_add_switch(o, "s2", "s2");
    
    if (optin_has_option(o, "xyzzy"))   {
        fprintf(stderr, "ERROR: optin_has_option() returned positive for test of non-existent option\n");
        goto done;
    }
    
    ret = optin_process(o, &argc, argv);
    
    switch(test)    {
    case 1:
        if (ival2 != 10)    {
            fprintf(stderr, "ival2 is %d and should be 10\n", ival2);
            ret = -1;
            break;
        }
        if (fval2 - 3.14 > 0.0001)  {
            fprintf(stderr, "fval2 is %f and should be 3.14\n", fval2);
            ret = -1;
            break;
        }
        if (strcmp(strval2, "this is a string"))   {
            fprintf(stderr, "strval2 is '%s' and should be 'this is a string'\n", strval2);
            ret = -1;
            break;
        }
        if (flagval1 != 1)  {
            fprintf(stderr, "flagval1 is %d and should be 1\n", flagval1);
            ret = -1;
            break;
        }
        ret = 0;
        break;
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