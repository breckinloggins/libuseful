#include "../test_utils.h"
#include "../include/platform.h"

DEFINE_TEST_FUNCTION {  
    char* src = "This is a copied string";
    char* dst;
    
    // xp_strdup
    dst = xp_strdup(src);
    if (!dst || dst == src || strcmp(dst, src)) {
        return -1;
    }
    free(dst);
    
    // xp_asprintf
    xp_asprintf(&dst, "This is %d %s string!\n", 1, "formatted");
    if (!dst || strcmp(dst, "This is 1 formatted string!\n"))   {
        return -1;
    }
    free(dst);
    
    return 0;
}

int main(int argc, char** argv) {
    RUN_TEST;
}