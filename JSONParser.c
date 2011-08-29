#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

#include "JSONTools.h"

/*----------------------------------------------------------------
 * Define private helper functions
 *---------------------------------------------------------------*/
 
static JSONError_t parseJSONString(JSONParser_t* parser, char* message, int size, char** result);
static JSONError_t parseJSONNumber(JSONParser_t* parser, char* message, int size, double* result);
static JSONError_t parseJSONBoolean(JSONParser_t* parser, char* message, int size, bool* result);
static JSONError_t parseJSONNull(JSONParser_t* parser, char* message, int size);
static JSONError_t parseJSONObject(JSONParser_t* parser, char* message, int size, JSONValue_t** result);
static JSONError_t parseJSONArray(JSONParser_t* parser, char* message, int size, JSONKeyValue_t** result);
static JSONError_t parseJSONKey(JSONParser_t* parser, char* message, int size);
static int pushProgress(JSONParser_t* parser, char* function);
static int popProgress(JSONParser_t* parser);

/*----------------------------------------------------------------
 * Implement global functions
 *--------------------------------------------------------------*/

/**
 * Creates a new parser object that can be used to parse a JSON message.
 * the JSON parser object maintains the parsers state, including the
 * current message depth, key stack, status, and verious statistics.
 * 
 * @return Pointer to new JSON parser object 
 */
JSONParser_t* newJSONParser(){
   JSONParser_t* newParser = (JSONParser_t*) malloc(sizeof(JSONParser_t));
   
   if (!newParser){
      PUSH_ERROR(INTERNAL_FAILURE, "Unable to allocate memory for JSONParser object");
      return NULL;
   }
   
   memset(newParser, 0, sizeof(JSONParser_t));
   
   //The first thing a new parser should see is a '{' character
   newParser->state = (NEW);  
         
   return newParser;
}

/**
 * Builds the document model for the specific JSON message that has 
 * come in. 
 * 
 * @param parser - The parser object that will be used to track the progress
 *    of parsing process
 * 
 * @param document - a pointer to the document, if the document
 *    is null, a new one will be built and accessable after the parser
 *    function has returned, if the document is not null, it's contents
 *    will be freed and a new document will be built.
 * 
 * @param message - the JSON message that you want to parse, a length is not
 *    necessary since it will parse up until the last '}' char, or '\0' 
 *    character. 
 * 
 * @return SUCCESS if the message was parsed correctly, an error otherwise.
 *    The problem will be pushed onto a stack and be be reterived, and used
 *    to debug the problem with the message.
 */
JSONError_t parseJSONMessage(JSONParser_t* parser, JSONKeyValue_t** document, char* message, int* lastIndex){
   if (!parser || !document || !message){
      PUSH_ERROR(NULL_ARGUMENT, "Expected a pointer, but found NULL");
      return NULL_ARGUMENT;
   }
   
   int messageLength = strlen(message);
   parser->index = 0;
   JSONError_t returnStatus;
   
   if (parser->state & NEW){
      //Seek forward to first '{' symbol
      while(parser->index < messageLength && message[parser->index] != '{'){
         //If we find something other then a space or '{', we have a problem
         if (isgraph(message[parser->index++])){
            char temp[50];
            sprintf(temp, "Looking for '{' but found '%c'", message[parser->index - 1]);
            PUSH_ERROR(INVALID_MESSAGE, temp);
            return INVALID_MESSAGE;
         }
      }
      
      parser->index++;  //move past the '{' character
      
      //If we never found the '{' and ran out of message
      if (parser->index >= messageLength){
         PUSH_ERROR(INVALID_MESSAGE, "Unable to find initial '{' before end of message");
         return INVALID_MESSAGE;
      }
      
      JSONValue_t* objectValue;
      returnStatus = parseJSONObject(parser, message, messageLength, &objectValue);
      if (returnStatus != SUCCESS){
         switch (returnStatus){
            case MESSAGE_INCOMPLETE :
               PUSH_ERROR(MESSAGE_INCOMPLETE, "The parsed message was found to be incomplete, please run again with the rest of the message");
               return MESSAGE_INCOMPLETE;
               break;
            default :
               PUSH_ERROR(PARSER_ERROR, "Unable to parse message, there are unresolved errors");
               return PARSER_ERROR;
               break;
         }
      }
      
      
      *document = newJSONPair(OBJECT, NULL, objectValue);
      
   }
   else if (parser->state != NEW && parser->lastStatus == MESSAGE_INCOMPLETE){
      //Complete the message parsing here.
   }
   else{
      //Some weird state, need to reject untill corrected. 
   }
   
   return SUCCESS;
}

/*-----------------------------------------------------------------
 * Implement private helper functions
 *----------------------------------------------------------------*/

/**
 * This is a helper function to parse a single string value out of the
 * message and into memory. The string created by this function
 * is dynamicly allocated, and needs to be freed when you are done with it
 * 
 * @param parser - The parser object that is keeping track of this specific document
 * @param message - The JSON message 
 * @param size - The length of the message
 * @param result - The string that is parsed out will be put here
 * @return SUCCESS if the string was parsed correctly, error otherwise (see stack trace)
 */
static JSONError_t parseJSONString(JSONParser_t* parser, char* message, int size, char** result) {
   int tempSize = 30;
   char* temp = (char*) malloc(sizeof(char) * (tempSize + 1));
   
   if(!temp){
      PUSH_ERROR(INTERNAL_FAILURE, "Unable to allocate memory for string object");
      return INTERNAL_FAILURE;
   }
   
   int tempIndex = 0;
   
   //Scan and copy the string into memory exactly as is
   while(parser->index < size && message[parser->index] != '"'){
      //Since the JSON standard states that we can have escaped sequences, we need
      //To copy them to memory correctly. 
      if (message[parser->index] == '\\'){
         //We can have escaped unicode sequences of the style '\uXXXX'
         if (message[parser->index + 1] == 'u' || message[parser->index + 1] == 'U'){
            if (tempIndex + 6 >= tempSize){
               tempSize *= 2;
               temp = (char*)realloc(temp, tempSize + 1);
               if (!temp){
                  PUSH_ERROR(INTERNAL_FAILURE, "Unable to expand memory for string object");
                  return INTERNAL_FAILURE;
               }
               continue;   //Now that we have more memory, continue to parse the string
            }
            
            temp[tempIndex++] = message[parser->index++];   //'/'
            temp[tempIndex++] = message[parser->index++];   //'u'
            for (int i = 0; i < 4; i++){
               if (isxdigit(message[parser->index])){
                  temp[tempIndex++] = message[parser->index++];   //Hex
               }
               else {
                  PUSH_ERROR(UNEXPECTED_VALUE, "Invalid Unicode escape sequence found while parsing string");
                  return UNEXPECTED_VALUE;
               }
            }
            
            continue; //Since we are already 1 character past the end of the escape sequence, we can continue parsing from here
         }
         else{
            if (tempIndex + 2 >= tempSize){
               tempSize *= 2;
               temp = (char*)realloc(temp, tempSize + 1);
               if (!temp){
                  PUSH_ERROR(INTERNAL_FAILURE, "Unable to expand memory for string object");
                  return INTERNAL_FAILURE;
               }
               continue;   //Now that we have more memory, continue to parse the string
            }
         }
         
         temp[tempIndex++] = message[parser->index++];   //'/'
         temp[tempIndex++] = message[parser->index];     //escaped character
      }
      //For normal characters, we just copy them into memory
      else {
         if (tempIndex + 1 >= tempSize){
            tempSize *= 2;
            temp = (char*)realloc(temp, tempSize + 1);
            if (!temp){
               PUSH_ERROR(INTERNAL_FAILURE, "Unable to expand memory for string object");
               return INTERNAL_FAILURE;
            }
            continue;   //Now that we have more memory, continue to parse the string
         }
         
         temp[tempIndex++] = message[parser->index];
      }
      
      parser->index++;
   }
   
   if (parser->index >= size){
      free(temp);
      PUSH_ERROR(UNEXPECTED_EOF, "Parsing incomplete, hit end of file while reading string object");
      return UNEXPECTED_EOF;
   }
   
   temp[tempIndex] = '\0';
   
   tempSize = strlen(temp);
   
   if (tempSize <= 0){
      tempSize = 1;
   }
   
   //shrink the memory of the string to only what it needs
   temp = (char*)realloc(temp, sizeof(char) * tempSize + 1);
   *result = temp;
   
   return SUCCESS;
}
/**
 * This is a helper function to parse a number from a JSON message. the 
 * JSON standard states that all numbers should be treated like IEEE 753 64 bit
 * double percesion numbers. That is exatly how C doubles work. They can 
 * be in either normal, or scientific notation. They cannot however be in
 * base 16 (which makes no sence for a double anyway).
 * 
 * @param parser - The parser object that is keeping track of this specific document
 * @param message - The JSON message 
 * @param size - The length of the message
 * @param result - The number that is parsed out will be put here
 * @return SUCCESS if the number was parsed correctly, error otherwise (see stack trace)
 */
static JSONError_t parseJSONNumber(JSONParser_t* parser, char* message, int size, double* result){
   int tempSize = 48;
   char temp[tempSize];
   int tempIndex = 0;
   
   while(parser->index < size && tempIndex < tempSize){
      if (isdigit(message[parser->index])){
         temp[tempIndex++] = message[parser->index];
      }
      else if (message[parser->index] == '-'){
         temp[tempIndex++] = message[parser->index];
      }
      else if (message[parser->index] == '+'){
         temp[tempIndex++] = message[parser->index];
      }
      else if (message[parser->index] == 'e'){
         temp[tempIndex++] = message[parser->index];
      }
      else if (message[parser->index] == 'E'){
         temp[tempIndex++] = message[parser->index];
      }
      else if (message[parser->index] == '.'){
         temp[tempIndex++] = message[parser->index];
      }
      else {
         //Need to step back to last character to ensure we dont skip any commas
         //or bracket characters
         parser->index--;
         break;
      }
      
      parser->index++;
   }
   
   if (parser->index >= size){
      PUSH_ERROR(MESSAGE_INCOMPLETE, "Hit the end of the message while parsing number value");
      return MESSAGE_INCOMPLETE;
   }
   
   temp[tempIndex] = '\0';
   
   errno = 0;
   double newNum = strtod(temp, NULL);
   
   if (errno){
      if (errno == ERANGE){
         PUSH_ERROR(NUMBER_OUT_OF_RANGE, "The number is out of range for the double type");
         return NUMBER_OUT_OF_RANGE;
      }
      else{
         char error[50];
         printf(error, "Unknown error, unable to convert number. (errno = %d)", errno);
         PUSH_ERROR(INTERNAL_FAILURE, error);
         return INTERNAL_FAILURE;
      }
      
   }
   
   *result = newNum;
   return SUCCESS;
}

/**
 * This is a helper function that will parse boolean values from a JSON message.
 * a boolean can only be 'true' or 'false' and no other values. 
 * 
 * @param parser - The parser object that is keeping track of this specific document
 * @param message - The JSON message 
 * @param size - The length of the message
 * @param result - The boolean that is parsed out will be put here
 * @return SUCCESS if the boolean was parsed correctly, error otherwise (see stack trace)
 */
static JSONError_t parseJSONBoolean(JSONParser_t* parser, char* message, int size, bool* result){
   int tempSize = 6;
   char temp[tempSize + 1];
   int tempIndex = 0;
   
   while(parser->index < size && tempIndex < tempSize){
      if (isalpha(message[parser->index])){
         temp[tempIndex++] = message[parser->index];
      }
      else {
         //Need to step back to last letter to ensure we dont skip any commas
         //or bracket characters
         parser->index--;
         break;
      }
      parser->index++;
   }
   
   if (parser->index >= size){
      PUSH_ERROR(MESSAGE_INCOMPLETE, "Hit the end of the message while parsing boolean value");
      return MESSAGE_INCOMPLETE;
   }
   
   temp[tempIndex] = '\0';
   
   if (strcasecmp(temp, "true") == 0){
      *result = true;
      return SUCCESS;
   }
   else if (strcasecmp(temp, "false") == 0){
      *result = false;
      return SUCCESS;
   }
   else {
      //not true or false
      char error[40];
      sprintf(error, "found \"%s\", expected true or false", temp);
      PUSH_ERROR(UNEXPECTED_VALUE, error);
      return UNEXPECTED_VALUE;
   }
   
}

/**
 * This function is special since we only need to know if it sucessfully parsed
 * the null value or not. Therefor there is no value to be returned. 
 * 
 * @param parser - The parser object that is keeping track of this specific document
 * @param message - The JSON message 
 * @param size - The length of the message
 * @return SUCCESS if the null was parsed correctly, error otherwise (see stack trace)
 */
static JSONError_t parseJSONNull(JSONParser_t* parser, char* message, int size) {
   int tempSize = 5;
   char temp[tempSize + 1];
   int tempIndex = 0;
   
   while(parser->index < size && tempIndex < tempSize) {
      if (isalpha(message[parser->index])) {
         temp[tempIndex++] = message[parser->index];
      }
      else {
         //Need to step back to last letter to ensure we dont skip any commas
         //or bracket characters
         parser->index--;
         break;
      }
      
      parser->index++;
   }
   
   if (parser->index >= size){
      PUSH_ERROR(MESSAGE_INCOMPLETE, "Hit the end of the message while parsing null value");
      return MESSAGE_INCOMPLETE;
   }
   
   temp[tempIndex] = '\0';
   
   if (strcasecmp(temp, "null") == 0) {
      return SUCCESS;
   }
   
   else {
      char error[35];
      sprintf(error, "found \"%s\", expected null", temp);
      PUSH_ERROR(UNEXPECTED_VALUE, error);
      return UNEXPECTED_VALUE;
   }
   
}

/**
 * This helper function will parse out a nested JSON object. the JSON standard
 * states that an object contains a set of unordered key:value pairs. Those
 * pairs could include other nested objects. This JSON parser only allows
 * 256 layers of depth. 
 * 
 * @param parser - The parser object that is keeping track of this specific document
 * @param message - The JSON message 
 * @param size - The length of the message
 * @param result - The object that is parsed out will be put here
 * @return SUCCESS if the object was parsed correctly, error otherwise (see stack trace)
 */
static JSONError_t parseJSONObject(JSONParser_t* parser, char* message, int size, JSONValue_t** result) {
   
   if (parser->depth >= MAX_DEPTH){
      PUSH_ERROR(MESSAGE_TOO_LARGE, "The maximum amount of nested objects has been exceeded");
      return MESSAGE_TOO_LARGE;
   }
   
   JSONValue_t* newObj = (JSONValue_t*)calloc(1, sizeof(JSONValue_t));
   
   if (!newObj){
      PUSH_ERROR(INTERNAL_FAILURE, "Unable to allocate memory for JSON object type");
      return INTERNAL_FAILURE;
   }
   
   JSONError_t returnStatus;
   
   parser->state &= CLEAR_STATE;
   parser->state |= (KEY | QUOTE | CLOSE_PREN | NULL_VALUE);
   
   while(parser->index < size){
      if (isspace(message[parser->index])){
         //we can ignore white space
      }
      else if (isalpha(message[parser->index])){
         //We found a boolean (should be true or false) or null, they are not quoted
         if (message[parser->index] == 'n'){
            if (parser->state & NULL_VALUE){
               //Found a null (maybe)
               returnStatus = parseJSONNull(parser, message, size);
               if (!returnStatus){
                  addKeyValuePair(newObj, newJSONPair(NIL, NULL, NULL));
                  
                  parser->state &= CLEAR_STATE;
                  parser->state |= (COMMA | CLOSE_PREN);
               }
               else {
                  PUSH_ERROR(PARSER_ERROR, "Error while trying to parse a null object");
                  return PARSER_ERROR;
               }
            }
            else {
               PUSH_ERROR(UNEXPECTED_VALUE, "Found an unexpected null value");
               return UNEXPECTED_VALUE;
            }
         }
         else if (message[parser->index] == 't' || message[parser->index] == 'f'){
            if ((parser->state & CHARACTER) && (parser->state & VALUE)) {
               //Found boolean (maybe)
               bool boolVal;
               returnStatus = parseJSONBoolean(parser, message, size, &boolVal);
               if (!returnStatus){
                  JSONKeyValue_t* newPair = newJSONPair(BOOLEAN, parser->keyStack[parser->keyStackIndex - 1], newJSONBoolean(boolVal));
                  addKeyValuePair(newObj, newPair);
                  free(newPair);
                  parser->keyStackIndex--;
                  free(parser->keyStack[parser->keyStackIndex]);
                  parser->keyStack[parser->keyStackIndex] = NULL;
                  
                  parser->state &= CLEAR_STATE;
                  parser->state |= (COMMA | CLOSE_PREN);
               }
               else {
                  PUSH_ERROR(PARSER_ERROR, "Parser expected a boolean, but did not find one");
                  return PARSER_ERROR;
               }
            }
            else {
               PUSH_ERROR(UNEXPECTED_VALUE, "Unexpected boolean value found");
               return UNEXPECTED_VALUE;
            }
         }
         else {
            //Found some other random character that we weren't expecting
            PUSH_ERROR(UNEXPECTED_CHARACTER, "and unexpected character was found");
            return UNEXPECTED_CHARACTER;
         }
      }
      else if (isdigit(message[parser->index])){
         //We found a number value, they are not quoted
         if (parser->state & DIGIT){
            double value;
            returnStatus = parseJSONNumber(parser, message, size, &value);
            if (!returnStatus){
               JSONKeyValue_t* newPair = newJSONPair(NUMBER, parser->keyStack[parser->keyStackIndex - 1], newJSONNumber(value));
               addKeyValuePair(newObj, newPair);
               free(newPair);
               parser->keyStackIndex--;
               free(parser->keyStack[parser->keyStackIndex]);
               parser->keyStack[parser->keyStackIndex] = NULL;
               
               parser->state &= CLEAR_STATE;
               parser->state |= (COMMA | CLOSE_PREN);
            }
            else {
               PUSH_ERROR(PARSER_ERROR, "Invalid number found while parsing JSON object");
               return PARSER_ERROR;
            }
         }
         else {
            PUSH_ERROR(UNEXPECTED_NUMBER, "Found unexpected digit while parsing JSON object");
            return UNEXPECTED_NUMBER;
         }
      }
      else if (ispunct(message[parser->index])){
         if (message[parser->index] == '"'){
            //Found beginning of either a key or string value
            parser->index++;
            if (parser->state & KEY){
               //If we are expecting a key, we will read it in and place it on the stack
               returnStatus = parseJSONKey(parser, message, size);
               if (returnStatus){
                  PUSH_ERROR(PARSER_ERROR, "Error parsing key while parsing JSON object");
                  return PARSER_ERROR;
               }
               
               parser->state &= CLEAR_STATE;
               parser->state |= (DELIMITER);
            }
            else if (parser->state & VALUE) {
               char* value;
               returnStatus = parseJSONString(parser, message, size, &value);
               if (!returnStatus){
                  JSONKeyValue_t* newPair = newJSONPair(STRING, parser->keyStack[parser->keyStackIndex - 1], newJSONString(value));
                  addKeyValuePair(newObj, newPair);
                  free(newPair);
                  free(value);
                  parser->keyStackIndex--;
                  free(parser->keyStack[parser->keyStackIndex]);
                  parser->keyStack[parser->keyStackIndex] = NULL;
                  
                  parser->state &= CLEAR_STATE;
                  parser->state |= (COMMA | CLOSE_PREN);
               }
               else {
                  PUSH_ERROR(PARSER_ERROR, "Error parsing string while parsing JSON object");
                  return PARSER_ERROR;
               }
            }
            else {
               PUSH_ERROR(UNEXPECTED_STRING, "An unexpected '\"' character found while parsing a JSON object");
               return UNEXPECTED_STRING;
            }
         }
         else if (message[parser->index] == '{') {
            //Found the beginning of a JSON object
            if ((parser->state & VALUE) && (parser->state & OPEN_PREN)){
               parser->depth++;
               parser->index++;
               JSONValue_t* objVal;
               returnStatus = parseJSONObject(parser, message, size, &objVal);
               if (!returnStatus){
                  JSONKeyValue_t* newPair = newJSONPair(OBJECT, parser->keyStack[parser->keyStackIndex - 1], objVal);
                  addKeyValuePair(newObj, newPair);
                  free(newPair);
                  parser->keyStackIndex--;
                  free(parser->keyStack[parser->keyStackIndex]);
                  parser->keyStack[parser->keyStackIndex] = NULL;
                  
                  parser->state &= CLEAR_STATE;
                  parser->state |= (COMMA | CLOSE_PREN);
               }
               else {
                  PUSH_ERROR(PARSER_ERROR, "Unable to parser JSON object");
                  return PARSER_ERROR;
               }
            }
            else {
               PUSH_ERROR(UNEXPECTED_OBJECT, "an unexpected '{' character found while parsing a JSON object");
               return UNEXPECTED_OBJECT;
            }
         }
         else if (message[parser->index] == ':'){
            //Found the delimiter between a key:value pair
            if (parser->state & DELIMITER){
               parser->state &= CLEAR_STATE;
               parser->state |= (VALUE | QUOTE | OPEN_PREN | OPEN_BRACKET | DIGIT | CHARACTER);
            }
            else {
               PUSH_ERROR(UNEXPECTED_DELIMITER, "Unexpected ':' character found while parsing a JSON object");
               return UNEXPECTED_DELIMITER;
            }
         }
         else if (message[parser->index] == '['){
            //Found the beginning of an array
            if ((parser->state & VALUE) && (parser->state & OPEN_BRACKET)){
               parser->index++;
               JSONKeyValue_t* arrVal;
               returnStatus = parseJSONArray(parser, message, size, &arrVal);
               if (!returnStatus){
                  addKeyValuePair(newObj, arrVal);
                  free(arrVal);
                  parser->state &= CLEAR_STATE;
                  parser->state |= (COMMA | CLOSE_PREN);
               }
               else {
                  PUSH_ERROR(PARSER_ERROR, "Error while parsing array object");
                  return PARSER_ERROR;
               }
            }
         }
         else if (message[parser->index] == '}'){
            if (parser->state & CLOSE_PREN){
               //Done here
               parser->depth--;
               break;
            }
            else {
               PUSH_ERROR(UNEXPECTED_CHARACTER, "Unexpected '}' character found while parsing object value");
               return UNEXPECTED_CHARACTER;
            }
         }
         else if (message[parser->index] == ','){
            if (parser->state & COMMA){
               parser->state &= CLEAR_STATE;
               parser->state |= (KEY | QUOTE | NULL_VALUE);
            }
            else {
               PUSH_ERROR(UNEXPECTED_CHARACTER, "Unexpected ',' character found while parsing object value");
               return UNEXPECTED_CHARACTER;
            }
         }
      }
      
      parser->index++;
   }
   
   if (parser->index >= size){
      PUSH_ERROR(MESSAGE_INCOMPLETE, "Hit the end of the message while parsing object value");
      return MESSAGE_INCOMPLETE;
   }
   
   *result = newObj;
   return SUCCESS;
}

/**
 * This helper function will parse out a JSON array value from the JSON message.
 * The JSON standard states that a JSON array is an ordered collection of
 * values without keys. They can be a mix of types. 
 * 
 * @param parser - The parser object that is keeping track of this specific document
 * @param message - The JSON message 
 * @param size - The length of the message
 * @param result - The array that is parsed out will be put here
 * @return SUCCESS if the array was parsed correctly, error otherwise (see stack trace)
 */
static JSONError_t parseJSONArray(JSONParser_t* parser, char* message, int size, JSONKeyValue_t** result){
   int arraySize = 12;
   void** elements = (void**) malloc(sizeof(void*) * (arraySize + 1));
   JSONType_t* types = (JSONType_t*) malloc(sizeof(JSONType_t) * (arraySize + 1));
   int index = 0;
   JSONError_t returnStatus;
   
   JSONKeyValue_t* array;
   
   if (!elements || !types){
      PUSH_ERROR(INTERNAL_FAILURE, "Unable to allocate memory for JSON array object elements");
      return INTERNAL_FAILURE;
   }
   
   parser->state &= CLEAR_STATE;
   parser->state |= (VALUE | QUOTE | CHARACTER | DIGIT | CLOSE_BRACKET | OPEN_PREN | NULL_VALUE);
   
   while(parser->index < size){
      if (isspace(message[parser->index])){
         //we can ignore white space
      }
      else if (isalpha(message[parser->index])){
         if (message[parser->index] == 'n'){
            if (parser->state & NULL_VALUE){
               returnStatus = parseJSONNull(parser, message, size);
               if (!returnStatus){
                  elements[index] = NULL;
                  types[index] = NIL;
                  index++;
                  
                  parser->state &= CLEAR_STATE;
                  parser->state |= (COMMA | CLOSE_BRACKET);
               }
               else {
                  PUSH_ERROR(PARSER_ERROR, "Invalid null value found while parsing JSON array");
                  return PARSER_ERROR;
               }
            }
            else {
               PUSH_ERROR(UNEXPECTED_VALUE, "Found unexpected null value while parsing JSON array");
               return UNEXPECTED_VALUE;
            }
         }
         else if (message[parser->index] == 't' || message[parser->index] == 'f'){
            //We found a boolean (should be true or false) they are not quoted
            if (parser->state & VALUE && parser->state & CHARACTER){
               bool* boolVal = (bool*)malloc(sizeof(bool));
               returnStatus = parseJSONBoolean(parser, message, size, boolVal);
               if (returnStatus == SUCCESS){
                  elements[index] = boolVal;
                  types[index] = BOOLEAN;
                  index++;
                  
                  parser->state &= CLEAR_STATE;
                  parser->state |= (COMMA | CLOSE_BRACKET);
               }
               else {
                  PUSH_ERROR(PARSER_ERROR, "Invalid boolean found while parsing JSON array");
                  return PARSER_ERROR;
               }
            }
            else {
               PUSH_ERROR(UNEXPECTED_CHARACTER, "Found unexpected boolean while parsing JSON array");
               return UNEXPECTED_CHARACTER;
            }
         }
         else {
            char error[100];
            sprintf(error, "Found unexpected character '%c' while parsing JSON array", message[parser->index]);
            PUSH_ERROR(UNEXPECTED_CHARACTER, error);
            return UNEXPECTED_CHARACTER;
         }
      }
      else if (isdigit(message[parser->index])) {
         //We found a number value, they are not quoted
         if (parser->state & DIGIT){
            double* value = (double*)malloc(sizeof(double));
            returnStatus = parseJSONNumber(parser, message, size, value);
            if (!returnStatus){
               elements[index] = value;
               types[index] = NUMBER;
               index++;
               
               parser->state &= CLEAR_STATE;
               parser->state |= (COMMA | CLOSE_BRACKET);
            }
            else {
               PUSH_ERROR(PARSER_ERROR, "Invalid number found while parsing JSON array");
               return PARSER_ERROR;
            }
         }
         else {
            PUSH_ERROR(UNEXPECTED_NUMBER, "Found unexpected digit while parsing JSON array");
            return UNEXPECTED_NUMBER;
         }
      }
      else if (ispunct(message[parser->index])){
         if (message[parser->index] == '"') {
            //Found beginning of a string value (No keys in arrays)
            if (parser->state & QUOTE) {
               parser->index++;
               char* value;
               returnStatus = parseJSONString(parser, message, size, &value);
               if (!returnStatus) {
                  elements[index] = value;
                  types[index] = STRING;
                  index++;
                  
                  parser->state &= CLEAR_STATE;
                  parser->state |= (COMMA | CLOSE_BRACKET);
               }
               else {
                  PUSH_ERROR(PARSER_ERROR, "Error parsing string while parsing JSON array");
                  return PARSER_ERROR;
               }
            }
            else {
               PUSH_ERROR(UNEXPECTED_STRING, "An unexpected '\"' character found while parsing a JSON array");
               return UNEXPECTED_STRING;
            }
         }
         else if (message[parser->index] == '{') {
            //Found the beginning of a JSON object
            if ((parser->state & VALUE) && (parser->state & OPEN_PREN)){
               parser->depth++;
               parser->index++;
               JSONValue_t* objVal;
               returnStatus = parseJSONObject(parser, message, size, &objVal);
               if (!returnStatus){
                  elements[index] = objVal;
                  types[index] = OBJECT;
                  index++;
                  
                  parser->state &= CLEAR_STATE;
                  parser->state |= (COMMA | CLOSE_BRACKET);
               }
               else {
                  PUSH_ERROR(PARSER_ERROR, "Unable to parser JSON object");
                  return PARSER_ERROR;
               }
            }
            else {
               PUSH_ERROR(UNEXPECTED_OBJECT, "an unexpected '{' character found while parsing a JSON object");
               return UNEXPECTED_OBJECT;
            }
         }
         else if (message[parser->index] == '['){
            //Found the beginning of an array
            if ((parser->state & VALUE) && (parser->state & OPEN_BRACKET)){
               //Arrays create key-value pairs, but this array is a value of another array
               //we need to push a null key onto the stack for this array
               parser->keyStack[parser->keyStackIndex] = NULL;
               parser->keyStackIndex++;

               parser->index++;
               JSONKeyValue_t* arrVal;
               returnStatus = parseJSONArray(parser, message, size, &arrVal);
               if (!returnStatus){
                  elements[index] = arrVal;
                  types[index] = ARRAY;
                  index++;
                  
                  parser->state &= CLEAR_STATE;
                  parser->state |= (COMMA | CLOSE_BRACKET);
               }
               else {
                  PUSH_ERROR(PARSER_ERROR, "Error while parsing an enclosed array object");
                  return PARSER_ERROR;
               }
            }
         }
         else if (message[parser->index] == ']'){
            if (parser->state & CLOSE_BRACKET){
               //Done here
               break;
            }
            else{
               PUSH_ERROR(UNEXPECTED_CHARACTER, "Unexpected ']' character found while parsing array");
               return UNEXPECTED_CHARACTER;
            }
         }
         else if (message[parser->index] == ','){
            //Found seperator, we should expect another value
            parser->state &= CLEAR_STATE;
            parser->state |= (VALUE | QUOTE | CHARACTER | DIGIT | OPEN_PREN | OPEN_BRACKET | NULL_VALUE);
         }
         else {
            char error[100];
            sprintf(error, 
               "An unexpected character ('%c') was found while parsing the JSON array", 
               message[parser->index]);
            
            PUSH_ERROR(UNEXPECTED_CHARACTER, error);
            return UNEXPECTED_CHARACTER;
         }
      }
      
      if (index >= arraySize){
         arraySize *= 2;
         elements = (void**) realloc(elements, sizeof(void*) * (arraySize + 1));
         types = (JSONType_t*) realloc(types, sizeof(JSONType_t) * (arraySize + 1));
         
         if (!elements || ! types){
            PUSH_ERROR(INTERNAL_FAILURE, "Unable to allocate more space for JSON array elements");
            return INTERNAL_FAILURE;
         }
      }
      
      parser->index++;
   }
   
   if (parser->index >= size){
      PUSH_ERROR(MESSAGE_INCOMPLETE, "Hit the end of the message while parsing array value");
      return MESSAGE_INCOMPLETE;
   }
   
   array = newJSONArray(parser->keyStack[parser->keyStackIndex - 1], elements, types, index);
   
   //Need to free the booleans, and numbers, and strings
   for (int i = 0; i < array->length; i++){
      if (types[i] == NUMBER || types[i] == BOOLEAN || types[i] == STRING){
         free(elements[i]);
      }
   }
   
   free(elements);
   free(types);
   
   if (parser->keyStack[parser->keyStackIndex - 1]){
      free(parser->keyStack[parser->keyStackIndex - 1]);
   }
   parser->keyStack[parser->keyStackIndex - 1] = NULL;
   parser->keyStackIndex--;
   
   *result= array;
   return SUCCESS;
}

/**
 * This helper function will parse out an key for a key:value pair and 
 * push it onto the key stack of the parser. 
 * 
 * @param parser - The parser object that is keeping track of this specific document
 * @param message - The JSON message 
 * @param size - The length of the message
 * @return SUCCESS if the key was parsed correctly, error otherwise (see stack trace)
 */
static JSONError_t  parseJSONKey(JSONParser_t* parser, char* message, int size){
   char temp[0x100];
   int tempIndex = 0;
   
   //Keys can have escape sequences in them, we have to make sure they are
   //converted properly. 
   while(parser->index < size && message[parser->index] != '"') {
      if (message[parser->index] == '\\'){
         parser->index++;
         switch (message[parser->index]){
            case 'n':   //newline
               temp[tempIndex++] = '\n';
               break;
               
            case 't':   //tab
               temp[tempIndex++] = '\t';
               break;
            
            case 'b':   //backspace
               temp[tempIndex++] = '\b';
               break;
               
            case '\\':  //backslash
               temp[tempIndex++] = '\\';
               break;
               
            case 'f':   //formfeed
               temp[tempIndex++] = '\f';
               break;
               
            case '"':   //quote charcter
               temp[tempIndex++] = '"';
               break;
               
            default:
               return INVALID_MESSAGE;
         }
      }
      else{
         temp[tempIndex++] = message[parser->index];
      }
      
      parser->index++;
   }
   
   if (parser->index >= size){
      PUSH_ERROR(MESSAGE_INCOMPLETE, "Hit the end of the message while parsing JSON key");
      return MESSAGE_INCOMPLETE;
   }
   
   temp[tempIndex] = '\0';
   int tempSize = strlen(temp);
   //Create a string in the free store memory to hold the key
   char* key = (char*)calloc((tempSize + 1), sizeof(char));
   if (!key){
      PUSH_ERROR(INTERNAL_FAILURE, "Unable to allocate memeory for key");
      return INTERNAL_FAILURE;
   }
   
   //Copy the key and push it onto the stack
   strcpy(key, temp);
   if (parser->keyStackIndex < KEY_STACK_SIZE) { 
      parser->keyStack[parser->keyStackIndex++] = key;
   }
   else {
      return MESSAGE_TOO_LARGE;
   }
   
   return SUCCESS;
}
