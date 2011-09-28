#ifndef JSON_ERROR_H
#define JSON_ERROR_H

#include "JSONCommon.h"

#define PUSH_ERROR(error, description) (pushError(error, __func__, __FILE__, description))

/**
 * Define the common errors that can occure when working with JSON messages
 */
typedef enum {
   SUCCESS = 0,               /**< Everything went perfect */
   FAIL,                      /**< Unknown failure */
   NULL_ARGUMENT,             /**< The argument passed to a library function was NULL */
   NULL_VALUE,                /**< The value in a key:value pair was NULL when it should not have been */
   NULL_KEY,                  /**< The key in a key:value pair was NULL when it should not have been */
   INVALID_MESSAGE,           /**< The message is not valid JSON */
   INVALID_ARGUMENT,          /**< The argument passed to a library function is of the wrong type */
   INVALID_TYPE,              /**< The type specified in a pair does not exist */
   ILLEGAL_STRING_CHARACTER,  /**< The string contains characters that are not allowed by JSON */
   PARSER_ERROR,              /**< A recurrsive call faied, calling the current call to fail */
   ARRAY_BRACKET_MISMATCH,    /**< An extra closing bracket was found while parsing the message */
   OBJECT_BRACKET_MISMATCH,   /**< An extra closing brace was found while parsing the message */
   UNEXPECTED_EOF,            /**< The end of the message was reached in the middle of parsing */
   UNEXPECTED_NUMBER,         /**< A numeric value was found where there should not be one */
   UNEXPECTED_STRING,         /**< A string was found (quote really) where there should not be one */
   UNEXPECTED_VALUE,          /**< A value was found instead of a key */
   UNEXPECTED_KEY,            /**< A key was found instead of a value or delimiter */
   UNEXPECTED_OBJECT,         /**< An object '{' was found where there should not be one */
   UNEXPECTED_CHARACTER,      /**< An unquoted character was found where there should not be one */
   UNEXPECTED_ARRAY,          /**< An array '[' was found where there should not be one */
   UNEXPECTED_DELIMITER,      /**< a delimiter ':' was found where there should not be one */
   MESSAGE_TOO_LARGE,         /**< The message being parsed has over 256 nested objects */
   MESSAGE_INCOMPLETE,        /**< The message ends unexpectdly */
   NUMBER_OUT_OF_RANGE,       /**< The number value is out of range for a double type */
   NO_MATCHING_PAIR,          /**< The pair that was being searched for was not found */
   INTERNAL_FAILURE           /**< A stdlib function failed */
} JSONError_t;


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



/*--------------------------------------------------------------
 * Define global functions
 *-------------------------------------------------------------*/
 
#ifdef __cplusplus
extern "C" {
#endif

void pushError(JSONError_t error, const char* currentFunction, const char* currentFile, const char* description);
char* getErrorReport();
void clearErrorStack();
const char* json_strerror(int errno);

#ifdef __cplusplus
}
#endif

#endif
