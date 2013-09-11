#ifndef JSON_ERROR_H
#define JSON_ERROR_H

#include "jsoncommon.h"

/**
 * Define the common errors that can occure when working with JSON messages
 */
typedef enum {
   JSON_SUCCESS = 0,               /**< Everything went perfect */
   JSON_FAIL,                      /**< Unknown failure */
   JSON_BAD_PARSER_STATE,          /**< The parser is in a bad state, and cannot be used to parse this message */
   JSON_NULL_ARGUMENT,             /**< The argument passed to a library function was NULL */
   JSON_NULL_VALUE,                /**< The value in a key:value pair was NULL when it should not have been */
   JSON_NULL_KEY,                  /**< The key in a key:value pair was NULL when it should not have been */
   JSON_INVALID_MESSAGE,           /**< The message is not valid JSON (no '{' found?)*/
   JSON_INVALID_ARGUMENT,          /**< The argument passed to a library function is of the wrong type */
   JSON_INVALID_TYPE,              /**< The type specified in a pair does not exist */
   JSON_INVALID_UNICODE_SEQ,       /**< The unicode sequence was not valid */
   JSON_INVALID_VALUE,             /**< The value being parsed is not a valid for any type */
   JSON_INVALID_KEY,               /**< The key being parsed is not a valid json key */
   JSON_ILLEGAL_STRING_CHARACTER,  /**< The string contains characters that are not allowed by JSON */
   JSON_ARRAY_BRACKET_MISMATCH,    /**< An unexpected closing bracket was found while parsing the message */
   JSON_OBJECT_BRACKET_MISMATCH,   /**< An unexpected closing brace was found while parsing the message */
   JSON_UNEXPECTED_NUMBER,         /**< A numeric value was found where there should not be one */
   JSON_UNEXPECTED_STRING,         /**< A string was found (quote really) where there should not be one */
   JSON_UNEXPECTED_VALUE,          /**< A value was found instead of a key */
   JSON_UNEXPECTED_NULL,           /**< A null was found where it wasn't expected (perhapse after a key) */
   JSON_UNEXPECTED_BOOLEAN,        /**< A boolean value was found where it wasnt expected */
   JSON_UNEXPECTED_KEY,            /**< A key was found instead of a value or delimiter */
   JSON_UNEXPECTED_OBJECT,         /**< An object '{' was found where there should not be one */
   JSON_UNEXPECTED_CHARACTER,      /**< An unquoted character was found that was not 't' 'f' or 'n' */
   JSON_UNEXPECTED_ARRAY,          /**< An array '[' was found where there should not be one */
   JSON_UNEXPECTED_DELIMITER,      /**< a delimiter ':' was found where there should not be one */
   JSON_UNEXPECTED_COMMA,          /**< a comma ',' was found where there should not be one */
   JSON_MESSAGE_TOO_LARGE,         /**< The message being parsed has over 256 nested objects */
   JSON_MESSAGE_INCOMPLETE,        /**< The message ends unexpectdly */
   JSON_NUMBER_OUT_OF_RANGE,       /**< The number value is out of range for a double type */
   JSON_NO_MATCHING_PAIR,          /**< The pair that was being searched for was not found */
   JSON_MALLOC_FAIL,               /**< Unable to allocate memory for json object */
   JSON_INTERNAL_FAILURE           /**< A stdlib function failed */
} JSONError_t;

extern int json_errno;


/*--------------------------------------------------------------
 * Define global functions
 *-------------------------------------------------------------*/
 
#ifdef __cplusplus
extern "C" {
#endif

const char* json_strerror(int errNo);

#ifdef __cplusplus
}
#endif

#endif
