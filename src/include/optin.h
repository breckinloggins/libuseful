#ifndef OPTIN_H
#define OPTIN_H

#define OPTIN_HAS_DEFAULT           0
#define OPTIN_REQUIRED              1

#define OPTIN_ERR_INVALID_OPTION    -1
#define OPTIN_ERR_INVALID_VALUE     -2
#define OPTIN_ERR_VALUE_MISSING     -3
#define OPTIN_ERR_OPTION_MISSING    -4

typedef struct optin_tag optin;

/* Signature for a callback function that will be called if an option is found on the command line that
   has been registered as a custom type */
typedef void (*optin_fn)(optin* o, const char* name, const char* default_value, const char* value);

/**
 * Creates a new optin object.  By default, the new optin will accept the help option and print a 
 * suitable message about available options as well as the usage text, if such text is set with optin_set_usage
 */
optin* optin_new();

/**
 * Destroys the given optin object
 */
void optin_destroy(optin* o);

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
void optin_add_int(optin* o, const char* name, const char* description, int has_default, int* intptr);

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
void optin_add_flag(optin* o, const char* name, const char* description, int has_default, int* flagptr);

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
void optin_add_switch(optin* o, const char* name, const char* description);


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
void optin_add_float(optin* o, const char* name, const char* description, int has_default, float* floatptr);

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
void optin_add_string(optin* o, const char* name, const char* description, int has_default, char** stringptr);

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
void optin_set_callback(optin* o, const char* name, optin_fn callback);

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
void optin_set_shortname(optin* o, const char* name, char shortname);

/**
 * Sets the given usage text that will be shown when arguments do not match and as the top line of the
 * help
 */
void optin_set_usage_text(optin* o, const char* usage);

/**
 * Returns nonzero if the given optin object has an option by the given name, zero otherwise
 */
int optin_has_option(optin* o, const char* name);

/** 
 * Returns nonzero if the given option is present and was explicitly set by the user, zero if no
 * such option exists or if the option was not explcitly set by the user
 */
int optin_option_is_set(optin* o, const char* name);

/**
 * Processes the given option as if it had been given on the command line
 *
 * o        - The optin object that contains the option
 * opt      - The long or short option name (e.g. "velocity" or "v"), do not include dashes
 * value    - The value that the option takes (e.g "35").  Pass NULL if the option takes no value
 *
 * RETURNS: zero if the option was processed successfully, nonzero if there was an error
 */
int optin_process_option(optin* o, const char* opt, const char* value);

/**
 * Processes the given command line according to the configuration of the optin object
 *
 * o        - The optin object
 * argc     - Pointer to the argument count, should include the program name
 * argv     - Pointer to the arguments, argv[0] should be the program name
 *
 * On exit, argc and argv will be modified to be the arguments left over after option processing
 * RETURNS: zero if options were parsed successfully, nonzero if there was an error
 */
int optin_process(optin* o, int* argc, char** argv);

/**
 * Prints diagnostic information about the current state of the given optin object
 */
void optin_debug_print(optin* o);

#endif //OPTIN_H