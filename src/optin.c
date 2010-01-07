/**
 * optin - Command Line Options parser
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "platform.h"
#include "hashtable.h"
#include "optin.h"

/* Describes an option in the optin dictionary */
typedef struct _option_tag {
    char* name;
    char* description;
    
    int has_default;
    int accepts_value;
    optin_fn callback;
    
    enum {
        OPTION_NONE     = 0,        /* Not used externally, only used if the option hasn't yet been setup */
        OPTION_FLAG     ,       
        OPTION_SWITCH   ,
        OPTION_INT      ,
        OPTION_FLOAT    ,
        OPTION_STRING   ,
    } type;
    
    union   {
        void* valptr;           /* For when you know you have something, you just don't know what! */
        int* intptr;
        float* floatptr;
        char** stringptr;
    } value;
    
    int set;                    /* Set to 1 if the user has explicitly set the option */

} _option;

/* The object we will use in our hashtables, let's us hash an option multiple times by different keys
   (such as the long and short name) */
typedef struct _option_wrapper_tag  {
    char* key;
    int alias;                  /* Only one of the wrappers "owns" the option and can clean it up */
    
    _option* o;
} _option_wrapper;

/* Main object used in the optin API.  Exposed to users as an opaque handle object */
struct optin_tag    {
    hashtable* options;
     
    char* usage;
    
    int argc;
    char** argv;
};

/**
 * The hash function we will use for the option dictionary
 */
static int _option_wrapper_hash(const void* key)  {
    return ht_hashpjw(((_option_wrapper*)key)->key);
}

/**
 * The function we will use to determine if two options match
 */
static int _option_wrapper_match(const void* key1, const void* key2)  {
    return !strcmp(((_option_wrapper*)key1)->key, ((_option_wrapper*)key2)->key);
}

/**
 * The function that will be called when an option must be destroyed
 */
static void _option_wrapper_destroy(void *data)   {
    _option_wrapper* w = (_option_wrapper*)data;
    
    if (!w->alias)  {
        /* We own the option, therefore we must clean it up */
        free(w->o->name);
        free(w->o->description);
        free(w->o);
    }
    
    free(w->key);
    free(w);
}

/**
 * Looks up an option in the option dictionary by the given name.  Return NULL if not found
 */
static _option* _query(hashtable* options, const char* name)    {
    _option_wrapper* query_wrapper, *wrapper;

    if (!options)  {
       return 0;
    }

    query_wrapper = (_option_wrapper*)malloc(sizeof(_option_wrapper));
    memset(query_wrapper, 0, sizeof(_option_wrapper));
    query_wrapper->key = (char*)name;

    wrapper = query_wrapper;

    if (ht_lookup(options, (void*)&wrapper) != 0) {
        /* No value for this key */
        free(query_wrapper);
        return 0;
    }

    free(query_wrapper);

    return wrapper->o;
}

/**
 * Sets the one-character shortname of the given option
 *
 * options      - The options dictionary which contains the option
 * name         - The (full) name of the option
 * shortname    - The one-character short name of the option
 *
 * NOTES:
 *  - It is not necessary to call this function unless you wish to override the default short name, which
 *    is the first letter of the long name
 *  - If an existing short name exists (including the default), it will be removed and replaced
 *  - If an identical short name exists, it will be replaced with this one, even if it is for a different
 *    option
 */
void _set_shortname(hashtable* options, const char* name, char shortname)    {
    _option* option;
    _option_wrapper* wrapper, *query_wrapper;
    char* str_shortname;
    
    if (!options) {
        return;
    }
    
    option = _query(options, name);
    if (!option)    {
        return;
    }
    
    xp_asprintf(&str_shortname, "%c", shortname);
    
    wrapper = (_option_wrapper*)malloc(sizeof(_option_wrapper));
    wrapper->key = str_shortname;
    wrapper->alias = 1;             /* We don't own the option */
    query_wrapper = wrapper;
    if (ht_lookup(options, (void*)&wrapper) != 0)    {
        /* It's not in there, just add it */
        wrapper->key = xp_strdup(str_shortname);
        ht_insert(options, (void*)wrapper);
    } else {
        /* Already in there */
        free(query_wrapper->key);
        free(query_wrapper);
    }
    
    wrapper->o = option;
}

/**
 * Looks up an option in the option dictionary by the given name.  Creates a new option and adds it to
 * the options dictionary if not found
 */
static _option* _query_or_new(hashtable* options, const char* name)   {
    _option_wrapper* option_wrapper;
    _option* option;
    
    option = _query(options, name);
    if (!option)    {
        /* Not found, create and insert */
        option = (_option*)malloc(sizeof(_option));
        memset(option, 0, sizeof(_option));
        option->name = xp_strdup(name);
        
        /* Key by the long name */
        option_wrapper = (_option_wrapper*)malloc(sizeof(_option_wrapper));
        memset(option_wrapper, 0, sizeof(_option_wrapper));
        option_wrapper->key = xp_strdup(name);
        option_wrapper->alias = 0;              /* We own the option */
        option_wrapper->o = option;
        ht_insert(options, (void*)option_wrapper);
        
        /* Key by the short name */
        _set_shortname(options, name, name[0]);
    }
    
    return option;
}

/**
 * Adds the given option to the options list
 * 
 * options      - The options dictionary to which to add the option
 * name         - The long name of the option (example "velocity")
 * description  - The human readable description, used to print usage
 * has_default  - 1 if the value in intptr has a valid default at startup, 0 if a value must be supplied
 * option_type  - The data type of the option (OPTION_INT/OPTION_FLOAT/...)
 * valptr       - Pointer to a variable that will receive the parsed option
 *
 * NOTES:
 *  - Calling the function with a null options dictionary has no effect
 *  - If the option has already been added, it will be replaced
 */
static void _add_option(hashtable* options, const char* name, const char* description, int has_default, int option_type, void* valptr)   {
    _option* option;
    
    if (!options) {
        return;
    }
    
    option = _query_or_new(options, name);
    option->type = option_type;
    option->has_default = has_default;
    
    option->accepts_value = option->type == OPTION_INT || 
                            option->type == OPTION_FLOAT || 
                            option->type == OPTION_STRING;
    
    if (option->description) {
        /* Free previous description */
        free(option->description);
        option->description = 0;
    }
    
    if (description)    {
        option->description = xp_strdup(description);
    }
    
    option->value.valptr = valptr;
}

static void _help_fn(optin* o, const char* name, const char* default_value, const char* value)    {
    if (!o->usage) {
        /* TODO: Is there something sensible we could use here? */
        return;
    }
    
    fprintf(stdout, "%s\n", o->usage);
    
    /* TODO: Enumerate the options and print them all pretty like */
}


/**
 * Creates a new optin object.  By default, the new optin will accept the help option and print a 
 * suitable message about available options as well as the usage text, if such text is set with optin_set_usage
 */
optin* optin_new()  {
    optin* o = (optin*)malloc(sizeof(optin));
    memset(o, 0, sizeof(optin));
    
    o->options = (hashtable*)malloc(sizeof(hashtable));
    ht_init(o->options, 17, _option_wrapper_hash, _option_wrapper_match, _option_wrapper_destroy);
    
    optin_add_switch(o, "help", "Displays help for the program");
    optin_set_callback(o, "help", _help_fn);
    
    return o;
}

/**
 * Destroys the given optin object
 */
void optin_destroy(optin* o)    {
    ht_destroy(o->options);
    if (o->usage)   {
        free(o->usage);
    }
    free(o);
}

/**
 * Adds the given integer option to the options list
 * 
 * o            - The optin object to which to add the option
 * name         - The long name of the option (example "velocity")
 * description  - The human readable description, used to print usage
 * has_default  - OPTIN_HAS_DEFAULT if the option has a valid default at startup, OPTIN_REQURED
 *                if a value must be supplied
 * intptr       - Pointer to an integer that will receive the parsed option
 *
 * NOTES:
 *  - Calling the function with a null optin object has no effect
 *  - If the option has already been added, it will be replaced
 */
void optin_add_int(optin* o, const char* name, const char* description, int has_default, int* intptr)    {
    _add_option(o->options, name, description, has_default, OPTION_INT, (void*)intptr);
}

/**
 * Adds the given flag option to the options list
 * 
 * o            - The optin object to which to add the option
 * name         - The long name of the option (example "haswidth")
 * description  - The human readable description, used to print usage
 * has_default  - OPTIN_HAS_DEFAULT if the option has a valid default at startup, OPTIN_REQURED
 *                if a value must be supplied
 * flagptr      - Pointer to an integer that will receive the parsed option
 *
 * NOTES:
 *  - Calling the function with a null optin object has no effect
 *  - If the option has already been added, it will be replaced
 *  - It is an error to add a flag option that has a "no" prefix to an existing flag option
 */
void optin_add_flag(optin* o, const char* name, const char* description, int has_default, int* flagptr)    {
    /* TODO: Check for case where we're adding a "no" flag for an existing flag or we're adding a non-"no" flag
       that already has a no-flag */
       
    _add_option(o->options, name, description, has_default, OPTION_FLAG, (void*)flagptr);
}

/**
 * Adds the given switch option to the options list (see notes for how this differs from a flag)
 * 
 * o            - The optin object to which to add the option
 * name         - The long name of the option (example "help")
 * description  - The human readable description, used to print usage
 *
 * NOTES:
 *  - Calling the function with a null optin object has no effect
 *  - Switches differ from flags in that they have no value, they are either present or absent.  They are 
 *    mostly used for options like "help" in which the option is more of a command than an option.  Also, 
 *    unlike flags, switches have no -no pairs
 *  - If the option has already been added, it will be replaced
 *  - It is an error to add a flag option that has a "no" prefix to an existing flag option
 */
void optin_add_switch(optin* o, const char* name, const char* description)    {
    _add_option(o->options, name, description, OPTIN_HAS_DEFAULT, OPTION_SWITCH, 0);
}


/**
 * Adds the given float option to the options list
 * 
 * o            - The optin object to which to add the option
 * name         - The long name of the option (example "haswidth")
 * description  - The human readable description, used to print usage
 * has_default  - OPTIN_HAS_DEFAULT if the option has a valid default at startup, OPTIN_REQURED
 *                if a value must be supplied
 * floatptr      - Pointer to a float that will receive the parsed option
 *
 * NOTES:
 *  - Calling the function with a null optin object has no effect
 *  - If the option has already been added, it will be replaced
 */
void optin_add_float(optin* o, const char* name, const char* description, int has_default, float* floatptr)    {   
    _add_option(o->options, name, description, has_default, OPTION_FLOAT, (void*)floatptr);
}

/**
 * Adds the given string option to the options list
 * 
 * o            - The optin object to which to add the option
 * name         - The long name of the option (example "haswidth")
 * description  - The human readable description, used to print usage
 * has_default  - OPTIN_HAS_DEFAULT if the option has a valid default at startup, OPTIN_REQURED
 *                if a value must be supplied
 * stringptr    - Pointer to a char pointer that will receive the parsed option
 *
 * NOTES:
 *  - Calling the function with a null optin object has no effect
 *  - If the option has already been added, it will be replaced
 *  - If the existing string pointer is valid and has_default is true, it will be assumed to contain the
 *    default value.  After option parsing, the stringptr will point to a NEW string that has been allocated
 *    by optin, thus:
 *    1. It is your responsibility to retain a reference to the previous string pointer if you wish to keep it
 *    2. It is your responsibility to properly free the string pointed to by stringptr after options parsing
 *       has completed
 *    3. C Strings are hard, let's go shopping
 */
void optin_add_string(optin* o, const char* name, const char* description, int has_default, char** stringptr)    {   
    _add_option(o->options, name, description, has_default, OPTION_STRING, (void*)stringptr);
}

/**
 * Sets the callback function for the given option so that a user-function is called whenever an option
 * is set by the user
 * 
 * o            - The optin object to which to add the option
 * name         - The long name of the option (example "haswidth")
 * callback     - Pointer to a function of type optin_fn, which will be called if the given option appears
 *
 * NOTES:
 *  - Calling the function with a null optin object has no effect
 *  - If the option does not exist, this funciton has no effect
 *  - The function will NOT be called for the default value if the user did not explicitly include the option
 *    on the command line
 */
void optin_set_callback(optin* o, const char* name, optin_fn callback)    {   
    _option* option;
    
    if (!o) {
        return;
    }
    
    option = _query(o->options, name);
    if (!option)    {
        return;
    }
    
    option->callback - callback;
}

/**
 * Sets the one-character shortname of the given option
 *
 * o            - The optin object which contains the option
 * name         - The (full) name of the option
 * shortname    - The one-character short name of the option
 *
 * NOTES:
 *  - It is not necessary to call this function unless you wish to override the default short name, which
 *    is the first letter of the long name
 *  - If an existing short name exists (including the default), it will be removed and replaced
 *  - If an identical short name exists, it will be replaced with this one, even if it is for a different
 *    option
 */
void optin_set_shortname(optin* o, const char* name, char shortname)    {
    if (!o) {
        return;
    }
    
    _set_shortname(o->options, name, shortname);
}

/**
 * Sets the given usage text that will be shown when arguments do not match and as the top line of the
 * help
 */
void optin_set_usage_text(optin* o, const char* usage)  {
    if (!o) {
        return;
    }
    
    if (o->usage)   {
        free(o->usage);
    }
    
    if (usage)  {
        o->usage = xp_strdup(usage);
    }
}

/**
 * Returns nonzero if the given optin object has an option by the given name, zero otherwise
 */
int optin_has_option(optin* o, const char* name)    {
    if (!o) {
        return 0;
    }
    
    return _query(o->options, name) != 0;
}

/** 
 * Returns nonzero if the given option is present and was explicitly set by the user, zero if no
 * such option exists or if the option was not explcitly set by the user
 */
int optin_option_is_set(optin* o, const char* name)    {
    _option* option;
    
    if (!o) {
        return 0;
    }
    
    option = _query(o->options, name);
    return option && option->set;
}

/**
 * Processes the given option as if it had been given on the command line
 *
 * o        - The optin object that contains the option
 * opt      - The long or short option name (e.g. "velocity" or "v"), do not include dashes
 * value    - The value that the option takes (e.g "35").  Pass NULL if the option takes no value
 *
 * RETURNS: zero if the option was processed successfully, nonzero if there was an error
 */
int optin_process_option(optin* o, const char* opt, const char* value)  {
    _option* option;
    
    /*fprintf(stderr, "Processing option: %s, value: %s\n", opt, value);*/
    
    option = _query(o->options, opt);
    if (!option)    {
        fprintf(stderr, "Unrecognized option: %s\n", opt);
        return OPTIN_ERR_INVALID_OPTION;
    }
    
    switch(option->type)    {
    case OPTION_FLAG:
        if (option->value.intptr != 0)  {
            *option->value.intptr = 1;
        }
        break;
    case OPTION_INT:
        if (option->value.intptr != 0)  {
            *option->value.intptr = atoi(value);
        }
        break;
    case OPTION_FLOAT:
        if (option->value.floatptr != 0)    {
            *option->value.floatptr = atof(value);
        }
        break;
    case OPTION_STRING:
        if (option->value.stringptr != 0)   {
            *option->value.stringptr = xp_strdup(value);
        }
        break;
    }
    
    option->set = 1;
    
    return 0;
}

/**
 * Processes the given command line according to the configuration of the optin object
 *
 * o        - The optin object
 * argc     - Pointer to the argument count, should include the program name
 * argv     - Pointer to the arguments, *argv[0] should be the program name
 *
 * On exit, argc and argv will be modified to be the arguments left over after option processing
 * RETURNS: zero if options were parsed successfully, nonzero if there was an error
 */
int optin_process(optin* o, int* argc, char** argv) {
    int i, ret;
    int is_long_option;
    int next_argv;              /* Used to keep track of the next valid argv slot for shuffling non-option args */ 
    char* arg, *opt, *value;
    _option* option;
    _option_wrapper* wrapper;
    hashtable_iter* opt_iter;
    
    enum { STATE_NORMAL, STATE_IN_OPTION} state;
    
    is_long_option = 0;
    state = STATE_NORMAL;
    o->argc = *argc;
    o->argv = argv;
    next_argv = 1;
    
    ret = 0;
    i = 1;                      /* Skip over the program name */
    while (i < o->argc) {
        arg = o->argv[i];
        
        switch(state)   {
        case STATE_NORMAL:
            if (*arg == '-')    {
                state = STATE_IN_OPTION;
                opt = arg+1;
                continue;
            }
            argv[next_argv++] = o->argv[i];
            break;
        case STATE_IN_OPTION:
            (*argc)--;
            is_long_option = (*opt == '-');
            value = 0;
            if (is_long_option) {
                opt++;
                if (*opt == '\0')   {
                    /* We have a lone --, that means to stop arg processing NOW */
                    i++;
                    goto done;
                }
                
                /* We need to check if there's an equal sign in the longopt somewhere */
                value = opt;
                while (*value)  {
                    if (*value == '=' && is_long_option)  {
                        *value++ = '\0';      /* Modify string so opt ends at where equals sign was */
                        break;
                    } else if (*value == '=')    {
                        fprintf(stderr, "Equals sign not allowed in short option\n");
                        ret = OPTIN_ERR_INVALID_OPTION;
                        goto done;
                    }
                    value++;
                }
                
                if (!*value)    {
                    value = 0;
                }
            }
            
            if (*opt != '\0')   {   /* TODO: What do we do if it does? */
                option = _query(o->options, opt); 
                if (!option)    {
                    fprintf(stderr, "Unrecognized option: '%s'\n", opt);
                    ret = OPTIN_ERR_INVALID_OPTION;
                    goto done;
                } else if (value && !option->accepts_value) {
                    fprintf(stderr, "Option '%s' does not take a value\n", opt);
                    ret = OPTIN_ERR_INVALID_VALUE;
                    goto done;
                }
                
                if (option->accepts_value && !value)  {
                    /* Let's see if our next arg can function as a value */
                    if ((i + 1) == o->argc || *(o->argv[i+1]) == '-')  {
                        fprintf(stderr, "Option '%s' requires a value\n", opt);
                        ret = OPTIN_ERR_VALUE_MISSING; 
                        goto done;
                    } else {
                        (*argc)--;
                    }
                    
                    value = o->argv[i+1];                    
                }
                
                ret = optin_process_option(o, opt, value);
                if (ret != 0)   {
                    goto done;
                }
                
            }
            
            state = STATE_NORMAL;            
            break;
        }
        
        if (value && value == o->argv[i+1])    {
            /* Need to skip over the value we used */
            i += 2;
        } else {
            i++;
        }
    }
done:
    /* Analyze required options */
    opt_iter = ht_iter_begin(o->options);
    while (opt_iter)    {
        
        wrapper = ht_value(opt_iter);
        
        /* Only consider the real options, ignore aliases so we don't check the same option twice */
        if (wrapper && !wrapper->alias)   {
            option = wrapper->o;
            if (option && option->has_default == OPTIN_REQUIRED && !option->set) {
                fprintf(stderr, "Missing required option '%s'\n", option->name);
                ret = OPTIN_ERR_OPTION_MISSING;
                break;
            }
        }
        
        opt_iter = ht_iter_next(opt_iter);
    }
    
    /* Reorder any args after the options in the caller's argv array */
    for (; i < o->argc; i++)    {
        argv[next_argv++] = o->argv[i];
    }
    return ret;
}

/**
 * Prints diagnostic information about the current state of the given optin object to stderr
 */
void optin_debug_print(optin* o)    {
    int i;
    
    if (!o) {
        fprintf(stderr, "No optin object\n");
        return;
    } else if (!o->argv) {
        fprintf(stderr, "No argv set in optin object (have you called optin_process?)\n");
        return;
    }

    for (i = 0; i < o->argc; i++)   {
        fprintf(stderr, "%3d: %s\n", i, o->argv[i]);
    }
}