#ifndef _JSON_PARSER_H
#define _JSON_PARSER_H

#include "JSONCommon.h"
#include "JSONError.h"

#define CLEAR_STATE              0x00000000
#define CLEAR_ITEM               0xFFFFFF00
#define CLEAR_CHARACTER          0xFFFF00FF
#define MAX_DEPTH                256

/**
 * The ParserState type is designed to be used by the parser as a look
 * ahead state. The parser can be looking for three items, either a 
 * Key, Delimiter, or Value. Depending on which item is being looked for
 * the context of the found characters changes. These values are meant to 
 * be combined with OR's and AND's to form more complex representations
 * of what the parser is expecting to find next. By using that the parser
 * can detect syntax errors in the JSON message.
 */
typedef enum {
   NEW =             0x00000001, /**< A brand new parser */
   KEY =             0x00000002, /**< Looking for a key (Quoted string) */
   DELIMITER =       0x00000004, /**< Looking for ':' character */
   VALUE =           0x00000008, /**< Looking for a JSON value */
   
   DIGIT =           0x00000100, /**< Looking for first digit of JSON number */
   QUOTE =           0x00000200, /**< Looking for quote to signify a string value */
   CHARACTER =       0x00000400, /**< Looking for unquoted character (true, false, null) */
   COMMA =           0x00000800, /**< Looking for comma, if found then there are more pairs */
   OPEN_PREN =       0x00001000, /**< Looking for '{' character to signify beginning of object */
   CLOSE_PREN =      0x00002000, /**< Looking for '}' character to signify end of object */
   OPEN_BRACKET =    0x00004000, /**< Looking for '[' character to signify beginning of array */
   CLOSE_BRACKET =   0x00008000, /**< Looking for ']' character to signify end of array */
   
   RESUME =          0x00010000  /**< Resume a previously incomplete message */
   
} ParserState_t;

/**
 * The JSON Parser object is used to keep track of the document parsing process.
 * The parser maintains state information, and a key stack to maintain 
 * key value alignment. 
 */
typedef struct {
   int depth;     /**< Keeps track of how many brackets have been found */
   int index;     /**< The current position in the message string */
   int lineNumber;            /**< The current line number of the document being parsed */ 
   JSONError_t lastStatus;    /**< the last status reported by the parser */
   char* keyStack[KEY_STACK_SIZE];  /**< the key names in the key:value pairs */
   int keyStackIndex;         /**< where we are in the key stack */
   
   ParserState_t state;       /**< What are we looking for in the message */
   
   //Some basic statistics
   int messagesParsed;        /**< How many messages have been parsed with this parser */
   int keyValuesParsed;       /**< How many key:value pairs have been parsed */
   int incompleteMessages;    /**< How many incomplete messages were resumed */
} JSONParser_t;

/*------------------------------------------------------------------
 * Define global functions
 *----------------------------------------------------------------*/

#ifdef CPP
extern "C" {
#endif

JSONParser_t* newJSONParser();
void resetParser(JSONParser_t* parser);
JSONError_t parseJSONMessage(JSONParser_t* parser, JSONKeyValue_t** document, char* message, int* lastIndex);

#ifdef CPP
}
#endif

#endif
