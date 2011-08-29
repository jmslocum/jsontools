#ifndef _JSON_COMMON_H
#define _JSON_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define KEY_STACK_SIZE           256

struct _json_key_value_t;

/**
 * Define the datatypes that are valid in JSON messages
 */
typedef enum {
   NUMBER,  /**< double percision floating point format */
   STRING,  /**< double quoted unicode strings */
   BOOLEAN, /**< true or false */
   ARRAY,   /**< an ordered sequence of values, comma-separated and enclosed in square brackets. The values don't need to have the same type. */
   OBJECT,  /**< an unordered collection of key:value pairs, comma-separated and enclosed in curly braces; the key must be a string */
   NIL      /**< a NULL value */
} JSONType_t;

/**
 * Defines the value object that will encapsulate all of the possible
 * values. Only the value that is in use should have a value. 
 */
typedef union {
   double nVal;   /**< The numeric value will be stored here */
   char* sVal;    /**< The string value will be stored here */
   bool bVal;     /**< the boolean value will be stored here */
   struct _json_key_value_t* oVal; /**< The pairs contained in an object will be stored here */
   struct _json_key_value_t* aVal; /**< The values in the array will be stored here (no keys) */
} JSONValue_t;

/**
 * Define a type that encapsulates a single JSON key:value pair. The 
 * type defines what the value will be, If this is not an array, the 
 * length should be 1, however if it is an array, the length of the 
 * array should be noted in the length field.
 * 
 * OBJECTS should be marked with the number of objects contained inside 
 * of the value->oVal, the next value of the OBJECT should skip all of the 
 * objects contents.
 * 
 * NULL values will have the type NIL, and a NULL value pointer. the next
 * pointer should still point to the next element. 
 */
typedef struct _json_key_value_t {
   JSONType_t type;  /**< They type of data held by this pair */
   int length;       /**< The number of element under this pair (for OBJECT and ARRAY) */
   char* key;        /**< The unique identifier for this pair */
   JSONValue_t* value;  /**< The actual value (or sub-value for OBJECT and ARRAY) */
   struct _json_key_value_t* next; /**< The next element after this if there is one */
} JSONKeyValue_t;

#endif
