#ifndef JSON_ERROR_H
#define JSON_ERROR_H

#include "JSONCommon.h"

#define PUSH_ERROR(error, description) (pushError(error, __func__, __FILE__, description))

/**
 * Define the common errors that can occure when working with JSON messages
 */
typedef enum {
   SUCCESS =                     0x00000000, /**< Everything went perfect */
   FAIL =                        0xFFFFFFFF, /**< Unknown failure */
   NULL_ARGUMENT =               0x00000001, /**< The argument passed to a library function was NULL */
   NULL_VALUE =                  0x00000002, /**< The value in a key:value pair was NULL when it should not have been */
   NULL_KEY =                    0x00000004, /**< The key in a key:value pair was NULL when it should not have been */
   INVALID_MESSAGE =             0x00000008, /**< The message is not valid JSON */
   INVALID_ARGUMENT =            0x00000010, /**< The argument passed to a library function is of the wrong type */
   INVALID_TYPE =                0x00000020, /**< The type specified in a pair does not exist */
   ILLEGAL_STRING_CHARACTER =    0x00000040, /**< The string contains characters that are not allowed by JSON */
   PARSER_ERROR =                0x00000080, /**< A recurrsive call faied, calling the current call to fail */
   ARRAY_BRACKET_MISMATCH =      0x00000100, /**< An extra closing bracket was found while parsing the message */
   OBJECT_BRACKET_MISMATCH =     0x00000200, /**< An extra closing brace was found while parsing the message */
   UNEXPECTED_EOF =              0x00000400, /**< The end of the message was reached in the middle of parsing */
   UNEXPECTED_NUMBER =           0x00000800, /**< A numeric value was found where there should not be one */
   UNEXPECTED_STRING =           0x00001000, /**< A string was found (quote really) where there should not be one */
   UNEXPECTED_VALUE =            0x00002000, /**< A value was found instead of a key */
   UNEXPECTED_KEY =              0x00004000, /**< A key was found instead of a value or delimiter */
   UNEXPECTED_OBJECT =           0x00008000, /**< An object '{' was found where there should not be one */
   UNEXPECTED_CHARACTER =        0x00010000, /**< An unquoted character was found where there should not be one */
   UNEXPECTED_ARRAY =            0x00020000, /**< An array '[' was found where there should not be one */
   UNEXPECTED_DELIMITER =        0x00040000, /**< a delimiter ':' was found where there should not be one */
   MESSAGE_TOO_LARGE =           0x00080000, /**< The message being parsed has over 256 nested objects */
   MESSAGE_INCOMPLETE =          0x00100000, /**< The message ends unexpectdly */
   NUMBER_OUT_OF_RANGE =         0x00200000, /**< The number value is out of range for a double type */
   NO_MATCHING_PAIR =            0x00400000, /**< The pair that was being searched for was not found */
   INTERNAL_FAILURE =            0x00800000  /**< A stdlib function failed */
} JSONError_t;

int json_errno;

/**
 * Define the error stack type that will hold the necessary information
 * to debug, or descover any problems with JSON message being parsed.
 */
typedef struct {
   JSONError_t errors[KEY_STACK_SIZE]; /**< The error types that have occurred */
   char* functions[KEY_STACK_SIZE];    /**< The functions that the errors occurred in */
   char* files[KEY_STACK_SIZE];        /**< The files that the errors occurred in */
   char* descriptions[KEY_STACK_SIZE]; /**< Breif descriptions of the error */
   int stackIndex;                     /**< where we are in the stack */
} JSONErrorStack_t;

/**
 * The globally shared error stack
 */
JSONErrorStack_t json_errorStack;

/*--------------------------------------------------------------
 * Define global functions
 *-------------------------------------------------------------*/
 
#ifdef CPP
extern "C" {
#endif

void pushError(JSONError_t error, const char* currentFunction, const char* currentFile, const char* description);
char* getErrorReport();
void clearErrorStack();

#ifdef CPP
}
#endif

#endif
