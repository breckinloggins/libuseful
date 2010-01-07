CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(libuseful)

FIND_PROGRAM(DiffFound diff)
IF(DiffFound STREQUAL "DiffFound-NOTFOUND")
    MESSAGE(FATAL_ERROR "The diff tool is not found on your system.  This is required for testing!")
ENDIF(DiffFound STREQUAL "DiffFound-NOTFOUND")

FIND_PATH(LIBUSEFUL_INCLUDES 
			test_utils.h
			hashtable.h
			list.h
			optin.h
			platform.h
			stringbuilder.h
			${CMAKE_CURRENT_SOURCE_DIR}/include
)

INCLUDE_DIRECTORIES(${LIBUSEFUL_INCLUDES})

SET(useful_LIB_SRCS
    platform.c
	hashtable.c
    list.c
    stringbuilder.c
    optin.c
    include/test_utils.h
	include/platform.h
	include/hashtable.h
    include/list.h
    include/stringbuilder.h
	include/optin.h
)

ADD_LIBRARY(useful ${useful_LIB_SRCS})

ENABLE_TESTING()

ADD_EXECUTABLE(hashtable_test platform.c list.c hashtable.c testing/hashtable_test.c)
ADD_TEST(hashtable_0 ${EXECUTABLE_OUTPUT_PATH}/hashtable_test)

ADD_EXECUTABLE(stringbuilder_test platform.c stringbuilder.c testing/stringbuilder_test.c)
ADD_TEST(stringbuilder_0 ${EXECUTABLE_OUTPUT_PATH}/stringbuilder_test)

ADD_EXECUTABLE(platform_test platform.c testing/platform_test.c)
ADD_TEST(platform_0 ${EXECUTABLE_OUTPUT_PATH}/platform_test)

ADD_EXECUTABLE(optin_test list.c hashtable.c platform.c optin.c testing/optin_test.c)
ADD_TEST(optin_0 ${EXECUTABLE_OUTPUT_PATH}/optin_test --test=1 -fval2 3.14 -ival2 10 -strval2 "this is a string" -g)
