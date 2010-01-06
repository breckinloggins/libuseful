#include "../test_utils.h"
#include "../include/platform.h"
#include "../include/hashtable.h"

static int _hash(const void* key)  {
    return ht_hashpjw((char*)key);
}

static int _match(const void* key1, const void* key2)  {
    return !strcmp((char*)key1, (char*)key2);
}

static void _destroy(void *data)   {
    free((char*)data);
}

DEFINE_TEST_FUNCTION {  
    hashtable_iter* iter;
    hashtable* ht = (hashtable*)malloc(sizeof(hashtable));
    int i;
    
    if (ht_init(ht, 17, _hash, _match, _destroy) != 0) {
        fprintf(stderr, "Hashtable not initialized\n");
        return -1;
    }
    
    /* Test enumerating over an empty hash table */
    i = 0;
    iter = ht_iter_begin(ht);
    while(iter) {
        fprintf(stdout, "Value at %p\n", ht_value(iter));
        iter = ht_iter_next(iter);
        i++;
    }
    if (i > 0)  {
        fprintf(stderr, "Non-zero iterations on empty hashtable\n");
        return -1;
    }
    
    /* Add some stuff */
    if (ht_insert(ht, (void*)xp_strdup("Hello")) != 0)  {
        fprintf(stderr, "Error inserting item 1\n");
        return -1;
    }
    
    if (ht_insert(ht, (void*)xp_strdup(",")) != 0)  {
        fprintf(stderr, "Error inserting item 2\n");
        return -1;
    }
    
    if (ht_insert(ht, (void*)xp_strdup(" ")) != 0)  {
        fprintf(stderr, "Error inserting item 3\n");
        return -1;
    }
    
    if (ht_insert(ht, (void*)xp_strdup("World")) != 0)  {
        fprintf(stderr, "Error inserting item 4\n");
        return -1;
    }
            
    /* Attempt to iterate over the list */
    i = 0;
    iter = ht_iter_begin(ht);
    while (iter)    {
        fprintf(stdout, "%s", (char*)ht_value(iter));
        iter = ht_iter_next(iter);
        i++;
    }
    fprintf(stdout, "\n");
    if (i != 4) {
        fprintf(stderr, "Number of items is %d, should be 4\n", i);
        return -1;
    }
    
    ht_destroy(ht);
    return 0;
}

int main(int argc, char** argv) {
    RUN_TEST;
}