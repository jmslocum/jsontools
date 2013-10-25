#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

#include "jsontools.h"

/*----------------------------------------------------------------
 * Define private helper functions
 *---------------------------------------------------------------*/
 
static JSONError_t parseJSONString(JSONParser_t* parser, const char* message, int size, char** result);
static JSONError_t parseJSONNumber(JSONParser_t* parser, const char* message, int size, double* result);
static JSONError_t parseJSONBoolean(JSONParser_t* parser, const char* message, int size, bool* result);
static JSONError_t parseJSONNull(JSONParser_t* parser, const char* message, int size);
static JSONError_t parseJSONObject(JSONParser_t* parser, const char* message, int size, JSONValue_t** result);
static JSONError_t parseJSONArray(JSONParser_t* parser, const char* message, int size, JSONKeyValue_t** result);
static JSONError_t parseJSONKey(JSONParser_t* parser, const char* message, int size);
static void pushError(JSONParser_t* parser, JSONError_t error, const char* currentFunction, const char* currentFile, int line, int errNo);
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
      json_errno = JSON_MALLOC_FAIL;
      return NULL;
   }
   
   JSONError_t ret = initJSONParser(newParser);
   if (ret != JSON_SUCCESS){
      return NULL;
   }

   return newParser;
}

/**
   Initialize a parser so it is ready to begin parsing a message

   @param[out] - The parser that will be initialized
   @return JSON_SUCCESS on success, JSON_NULL_ARGUMENT if the
      parser is not a valid pointer
*/
JSONError_t initJSONParser(JSONParser_t* parser){
   if (!parser){
      json_errno = JSON_NULL_ARGUMENT;
      return JSON_NULL_ARGUMENT;
   }

   memset(parser, 0, sizeof(JSONParser_t));

   parser->state = (OPEN_PREN || OPEN_BRACKET);

   return JSON_SUCCESS;
}

/**
 * Resets the parser object so it can be used on another message. Although
 * the parseJSONMessage will reset the parser after a successful parsing it
 * will be necessary to reset the parser manually after an error occurs. 
 * this does not clear any of the accounting data out of the struct.
 * 
 * @param parser - The parser object that needs to be cleared
 */
void resetParser(JSONParser_t* parser){
   if (!parser){
      return;
   }
   
   initJSONParser(parser);
}

/**
   This will free the memory held by the parser if it was created
   with newJSONParser(). 

   @param[in] parser - The parser whos memory will be freed
*/
void disposeOfJSONParser(JSONParser_t* parser){
   if (!parser){
      return;
   }

   free(parser);
}

/**
 * Builds the document model for the specific JSON message that has 
 * come in. A message can be an Object '{ pairs }'. An Array '[ ... ]'
 * This will parse out the whole document and put a pointer
 * to the document in JSONKeyValue_t** document.
 * 
 * @param parser - The parser object that will be used to track the progress
 *    of parsing process
 * 
 * @param document - a pointer to the document, if the document
 *    is null, a new one will be built and accessable after the parser
 *    function has returned, if the document is not null, it's contents
 *    will be freed and a new document will be built.
 * 
 * @param message - the JSON message that you want to parse, The message MUST
 *    be null terminated.
 * 
 * @return JSON_SUCCESS if the message was parsed correctly, an error otherwise.
 */
JSONError_t parseJSONMessage(JSONParser_t* parser, JSONKeyValue_t** document, const char* message, int* lastIndex){
   if (!parser || !document || !message){
      PUSH_ERROR(parser, JSON_NULL_ARGUMENT, -1);
      json_errno = JSON_NULL_ARGUMENT;
      return JSON_NULL_ARGUMENT;
   }
   
   int messageLength = strlen(message);
   parser->index = 0;
   JSONError_t returnStatus;
   
   //Seek forward to first character in the message
   while(parser->index < messageLength){
      if (isgraph(message[parser->index])){
         parser->index++;
         if (message[parser->index - 1] == '/' && message[parser->index] == '*'){
            //Found a comment, we need to read ahead to get past it
            parser->index++;
            while(parser->index < messageLength){
               if (message[parser->index] == '*'){
                  parser->index++;
                  if (message[parser->index] == '/' && parser->index < messageLength){
                     //Found end of comment
                     parser->index++;
                     break;
                  }
               }
               else if (message[parser->index] == '\n'){
                  parser->index++;
                  parser->lineNumber++;
               }
               else {
                  parser->index++;
               }
            }
         }
         else if (message[parser->index - 1] == '/' && message[parser->index] == '/'){
            //Found a single line comment, parse until end of line
            while(parser->index < messageLength && message[parser->index++] != '\n');
            parser->lineNumber++;
         }
         else {
            //We found the first character in the message
            //rewind index to point to that character
            parser->index--;
            break;
         }
      }
      else if (message[parser->index] == '\n') {
         parser->index++;
         parser->lineNumber++;
      }
      else {
         parser->index++;
      }
   }
   
   //If we never found the first character and ran out of message
   if (parser->index >= messageLength){
      PUSH_ERROR(parser, JSON_INVALID_MESSAGE, -1);
      json_errno = JSON_INVALID_MESSAGE;
      return JSON_INVALID_MESSAGE;
   }
   
   //Check what kind of character it is. It can either be an
   //Array or an Object
   
   if (message[parser->index] == '{'){
      //Found an object
      parser->index++;
      JSONValue_t* objectValue;
      returnStatus = parseJSONObject(parser, message, messageLength, &objectValue);
      if (returnStatus != JSON_SUCCESS){
         switch (returnStatus){
            case JSON_MESSAGE_INCOMPLETE :
               parser->incompleteMessages++;
               return JSON_MESSAGE_INCOMPLETE;
               break;
            default :
               return returnStatus;
               break;
         }
      }
      //Attach the object as the final json document
      *document = newJSONPair(OBJECT, NULL, objectValue);
   }
   else if (message[parser->index] == '['){
      //Found an array
      parser->index++;
      JSONKeyValue_t* arrayValue;
      returnStatus = parseJSONArray(parser, message, messageLength, &arrayValue);
      if (returnStatus != JSON_SUCCESS){
         switch (returnStatus){
            case JSON_MESSAGE_INCOMPLETE :
               parser->incompleteMessages++;
               return JSON_MESSAGE_INCOMPLETE;
               break;
            default :
               return returnStatus;
               break;
         }
      }
      //Attach the array as the final json document
      *document = arrayValue;
   }
   
   //Look forward down the message to see if another JOSN message starts
   //if it does, report the index of that message in lastIndex
   parser->index++;  //Step past last '}' character
   while(parser->index < messageLength && message[parser->index] != '{' && message[parser->index] != '['){
      //If we find something other then a space or '{', no more messages
      if (isgraph(message[parser->index])){
         if (message[parser->index] == '/' && message[parser->index + 1] == '*'){
            //Found a comment, we need to read ahead to get past it
            parser->index += 2;
            while(parser->index < messageLength){
               if (message[parser->index++] == '*'){
                  if (message[parser->index] == '/'){
                     //Found end of comment
                     break;
                  }
               }
            }
         }
         else {
            break;
         }
      }
      parser->index++;
   }
   
   if (message[parser->index] == '{' || message[parser->index] == '['){
      *lastIndex = parser->index;
   }
   else {
      *lastIndex = -1;
   }
   
   resetParser(parser);
   parser->messagesParsed++;
   return JSON_SUCCESS;
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
 * @return JSON_SUCCESS if the string was parsed correctly, error otherwise (see stack trace)
 */
static JSONError_t parseJSONString(JSONParser_t* parser, const char* message, int size, char** result) {
   if (!parser){
      PUSH_ERROR(parser, JSON_NULL_ARGUMENT, -1);
      json_errno = JSON_NULL_ARGUMENT;
      return JSON_NULL_ARGUMENT;
   }
   
   int tempSize = 256;
   char* temp = (char*) calloc(sizeof(char), (tempSize + 1));
   
   if(!temp){
      PUSH_ERROR(parser, JSON_NULL_ARGUMENT, errno);
      json_errno = JSON_MALLOC_FAIL;
      return JSON_MALLOC_FAIL;
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
                  PUSH_ERROR(parser, JSON_MALLOC_FAIL, errno);
                  json_errno = JSON_MALLOC_FAIL;
                  return JSON_MALLOC_FAIL;
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
                  PUSH_ERROR(parser, JSON_INVALID_UNICODE_SEQ, -1);
                  json_errno = JSON_INVALID_UNICODE_SEQ;
                  return JSON_INVALID_UNICODE_SEQ;
               }
            }
            
            continue; //Since we are already 1 character past the end of the escape sequence, we can continue parsing from here
         }
         else{
            if (tempIndex + 2 >= tempSize){
               tempSize *= 2;
               temp = (char*)realloc(temp, tempSize + 1);
               if (!temp){
                  PUSH_ERROR(parser, JSON_MALLOC_FAIL, errno);
                  json_errno = JSON_MALLOC_FAIL;
                  return JSON_MALLOC_FAIL;
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
               PUSH_ERROR(parser, JSON_MALLOC_FAIL, errno);
               json_errno = JSON_MALLOC_FAIL;
               return JSON_MALLOC_FAIL;
            }
            continue;   //Now that we have more memory, continue to parse the string
         }
         
         temp[tempIndex++] = message[parser->index];
      }
      
      parser->index++;
   }
   
   if (parser->index >= size){
      free(temp);
      PUSH_ERROR(parser, JSON_MESSAGE_INCOMPLETE, -1);
      json_errno = JSON_MESSAGE_INCOMPLETE;
      return JSON_MESSAGE_INCOMPLETE;
   }
   
   temp[tempIndex] = '\0';
   
   tempSize = strlen(temp) + 1;
   
   //shrink the memory of the string to only what it needs
   temp = (char*)realloc(temp, sizeof(char) * tempSize);
   *result = temp;
   
   return JSON_SUCCESS;
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
 * @return JSON_SUCCESS if the number was parsed correctly, error otherwise (see stack trace)
 */
static JSONError_t parseJSONNumber(JSONParser_t* parser, const char* message, int size, double* result){
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
      PUSH_ERROR(parser, JSON_MESSAGE_INCOMPLETE, -1);
      json_errno = JSON_MESSAGE_INCOMPLETE;
      return JSON_MESSAGE_INCOMPLETE;
   }
   
   temp[tempIndex] = '\0';
   
   errno = 0;
   double newNum = strtod(temp, NULL);
   
   if (errno){
      if (errno == ERANGE){
         PUSH_ERROR(parser, JSON_NUMBER_OUT_OF_RANGE, errno);
         json_errno = JSON_NUMBER_OUT_OF_RANGE;
         return JSON_NUMBER_OUT_OF_RANGE;
      }
      else{
         PUSH_ERROR(parser, JSON_INTERNAL_FAILURE, errno);
         json_errno = JSON_INTERNAL_FAILURE;
         return JSON_INTERNAL_FAILURE;
      }
      
   }
   
   *result = newNum;
   return JSON_SUCCESS;
}

/**
 * This is a helper function that will parse boolean values from a JSON message.
 * a boolean can only be 'true' or 'false' and no other values. 
 * 
 * @param parser - The parser object that is keeping track of this specific document
 * @param message - The JSON message 
 * @param size - The length of the message
 * @param result - The boolean that is parsed out will be put here
 * @return JSON_SUCCESS if the boolean was parsed correctly, error otherwise (see stack trace)
 */
static JSONError_t parseJSONBoolean(JSONParser_t* parser, const char* message, int size, bool* result){
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
      PUSH_ERROR(parser, JSON_MESSAGE_INCOMPLETE, -1);
      json_errno = JSON_MESSAGE_INCOMPLETE;
      return JSON_MESSAGE_INCOMPLETE;
   }
   
   temp[tempIndex] = '\0';
   
   if (strcmp(temp, "true") == 0){
      *result = true;
      return JSON_SUCCESS;
   }
   else if (strcmp(temp, "false") == 0){
      *result = false;
      return JSON_SUCCESS;
   }
   else {
      //not true or false
      PUSH_ERROR(parser, JSON_INVALID_VALUE, -1);
      json_errno = JSON_INVALID_VALUE;
      return JSON_INVALID_VALUE;
   }
   
}

/**
 * This function is special since we only need to know if it sucessfully parsed
 * the null value or not. Therefor there is no value to be returned. 
 * 
 * @param parser - The parser object that is keeping track of this specific document
 * @param message - The JSON message 
 * @param size - The length of the message
 * @return JSON_SUCCESS if the null was parsed correctly, error otherwise (see stack trace)
 */
static JSONError_t parseJSONNull(JSONParser_t* parser, const char* message, int size) {
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
      PUSH_ERROR(parser, JSON_MESSAGE_INCOMPLETE, -1);
      json_errno = JSON_MESSAGE_INCOMPLETE;
      return JSON_MESSAGE_INCOMPLETE;
   }
   
   temp[tempIndex] = '\0';
   
   if (strcmp(temp, "null") == 0) {
      return JSON_SUCCESS;
   }
   
   else {
      PUSH_ERROR(parser, JSON_INVALID_TYPE, -1);
      json_errno = JSON_INVALID_VALUE;
      return JSON_INVALID_VALUE;
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
 * @return JSON_SUCCESS if the object was parsed correctly, error otherwise (see stack trace)
 */
static JSONError_t parseJSONObject(JSONParser_t* parser, const char* message, int size, JSONValue_t** result) {
   if (parser->depth >= MAX_DEPTH){
      PUSH_ERROR(parser, JSON_MESSAGE_TOO_LARGE, -1);
      json_errno = JSON_MESSAGE_TOO_LARGE;
      return JSON_MESSAGE_TOO_LARGE;
   }
   
   JSONValue_t* newObj = (JSONValue_t*)calloc(1, sizeof(JSONValue_t));
   
   if (!newObj){
      PUSH_ERROR(parser, JSON_MALLOC_FAIL, errno);
      json_errno = JSON_MALLOC_FAIL;
      return JSON_MALLOC_FAIL;
   }
   
   JSONError_t returnStatus;
   
   
   parser->state &= CLEAR_STATE;
   parser->state |= (KEY | QUOTE | CLOSE_PREN | CHARACTER);
   
   while(parser->index < size){
      if (isspace(message[parser->index])){
         if (message[parser->index] == '\n'){
            parser->lineNumber++;
         }
         //we can ignore white space
      }
      else if (isalpha(message[parser->index])){
         //We found a boolean (should be true or false) or null, they are not quoted
         if (message[parser->index] == 'n'){
            if ((parser->state & CHARACTER) && (parser->state & VALUE)){
               //Found a null (maybe)
               returnStatus = parseJSONNull(parser, message, size);
               if (!returnStatus){
                  JSONKeyValue_t* newPair = newJSONPair(NIL, parser->keyStack[parser->keyStackIndex - 1], NULL);
                  addKeyValuePair(newObj, newPair);
                  free(newPair);
                  parser->keyStackIndex--;
                  free(parser->keyStack[parser->keyStackIndex]);
                  parser->keyStack[parser->keyStackIndex] = NULL;
                  
                  parser->state &= CLEAR_STATE;
                  parser->state |= (COMMA | CLOSE_PREN);
               }
               else {
                  return returnStatus;
               }
            }
            else {
               PUSH_ERROR(parser, JSON_UNEXPECTED_VALUE, -1);
               json_errno = JSON_UNEXPECTED_VALUE;
               return JSON_UNEXPECTED_VALUE;
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
                  return returnStatus;
               }
            }
            else {
               PUSH_ERROR(parser, JSON_UNEXPECTED_VALUE, -1);
               json_errno = JSON_UNEXPECTED_VALUE;
               return JSON_UNEXPECTED_VALUE;
            }
         }
         else {
            //Found some other random character that we weren't expecting
            PUSH_ERROR(parser, JSON_UNEXPECTED_CHARACTER, -1);
            json_errno = JSON_UNEXPECTED_CHARACTER;
            return JSON_UNEXPECTED_CHARACTER;
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
               return returnStatus;
            }
         }
         else {
            PUSH_ERROR(parser, JSON_UNEXPECTED_NUMBER, -1);
            json_errno = JSON_UNEXPECTED_NUMBER;
            return JSON_UNEXPECTED_NUMBER;
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
                  return returnStatus;
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
                  return returnStatus;
               }
            }
            else {
               PUSH_ERROR(parser, JSON_UNEXPECTED_STRING, -1);
               json_errno = JSON_UNEXPECTED_STRING;
               return JSON_UNEXPECTED_STRING;
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
                  return returnStatus;
               }
            }
            else {
               PUSH_ERROR(parser, JSON_UNEXPECTED_OBJECT, -1);
               json_errno = JSON_UNEXPECTED_OBJECT;
               return JSON_UNEXPECTED_OBJECT;
            }
         }
         else if (message[parser->index] == ':'){
            //Found the delimiter between a key:value pair
            if (parser->state & DELIMITER){
               parser->state &= CLEAR_STATE;
               parser->state |= (VALUE | QUOTE | OPEN_PREN | OPEN_BRACKET | DIGIT | CHARACTER);
            }
            else {
               PUSH_ERROR(parser, JSON_UNEXPECTED_DELIMITER, -1);
               json_errno = JSON_UNEXPECTED_DELIMITER;
               return JSON_UNEXPECTED_DELIMITER;
            }
         }
         else if (message[parser->index] == '['){
            //Found the beginning of an array
            if ((parser->state & VALUE) && (parser->state & OPEN_BRACKET)){
               parser->index++;
               JSONKeyValue_t* arrVal;
               returnStatus = parseJSONArray(parser, message, size, &arrVal);
               if (!returnStatus){
                  arrVal->key = calloc(strlen(parser->keyStack[parser->keyStackIndex - 1]) + 1, sizeof(char));
                  strcpy(arrVal->key, parser->keyStack[parser->keyStackIndex - 1]);
                  addKeyValuePair(newObj, arrVal);
                  free(arrVal);
                  parser->keyStackIndex--;
                  free(parser->keyStack[parser->keyStackIndex]);
                  parser->keyStack[parser->keyStackIndex] = NULL;
                  parser->state &= CLEAR_STATE;
                  parser->state |= (COMMA | CLOSE_PREN);
               }
               else {
                  return returnStatus;
               }
            }
         }
         else if (message[parser->index] == ']'){
            PUSH_ERROR(parser, JSON_ARRAY_BRACKET_MISMATCH, -1);
            json_errno = JSON_ARRAY_BRACKET_MISMATCH;
            return JSON_ARRAY_BRACKET_MISMATCH;
         }
         else if (message[parser->index] == '}'){
            if (parser->state & CLOSE_PREN){
               //Done here
               parser->depth--;
               break;
            }
            else {
               PUSH_ERROR(parser, JSON_OBJECT_BRACKET_MISMATCH, -1);
               json_errno = JSON_OBJECT_BRACKET_MISMATCH;
               return JSON_OBJECT_BRACKET_MISMATCH;
            }
         }
         else if (message[parser->index] == ','){
            if (parser->state & COMMA){
               parser->state &= CLEAR_STATE;
               parser->state |= (KEY | QUOTE | CHARACTER);
            }
            else {
               PUSH_ERROR(parser, JSON_UNEXPECTED_COMMA, -1);
               json_errno = JSON_UNEXPECTED_COMMA;
               return JSON_UNEXPECTED_COMMA;
            }
         }
      }
      
      parser->index++;
   }
   
   if (parser->index >= size){
      PUSH_ERROR(parser, JSON_MESSAGE_INCOMPLETE, -1);
      json_errno = JSON_MESSAGE_INCOMPLETE;
      return JSON_MESSAGE_INCOMPLETE;
   }
   
   *result = newObj;
   return JSON_SUCCESS;
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
 * @return JSON_SUCCESS if the array was parsed correctly, error otherwise (see stack trace)
 */
static JSONError_t parseJSONArray(JSONParser_t* parser, const char* message, int size, JSONKeyValue_t** result){
   int arraySize = 12;
   void** elements = (void**) malloc(sizeof(void*) * (arraySize + 1));
   JSONType_t* types = (JSONType_t*) malloc(sizeof(JSONType_t) * (arraySize + 1));
   int index = 0;
   JSONError_t returnStatus;
   
   JSONKeyValue_t* array;
   
   if (!elements || !types){
      PUSH_ERROR(parser, JSON_MALLOC_FAIL, errno);
      json_errno = JSON_MALLOC_FAIL;
      return JSON_MALLOC_FAIL;
   }
   
   parser->state &= CLEAR_STATE;
   parser->state |= (VALUE | QUOTE | CHARACTER | DIGIT | CLOSE_BRACKET | OPEN_PREN);
   
   while(parser->index < size){
      if (isspace(message[parser->index])){
         if (message[parser->index] == '\n'){
            parser->lineNumber++;
         }
         //we can ignore white space
      }
      else if (isalpha(message[parser->index])){
         if (message[parser->index] == 'n'){
            if ((parser->state & CHARACTER) && (parser->state & VALUE)){
               returnStatus = parseJSONNull(parser, message, size);
               if (!returnStatus){
                  elements[index] = NULL;
                  types[index] = NIL;
                  index++;
                  
                  parser->state &= CLEAR_STATE;
                  parser->state |= (COMMA | CLOSE_BRACKET);
               }
               else {
                  return returnStatus;
               }
            }
            else {
               PUSH_ERROR(parser, JSON_UNEXPECTED_NULL, -1);
               json_errno = JSON_UNEXPECTED_NULL;
               return JSON_UNEXPECTED_NULL;
            }
         }
         else if (message[parser->index] == 't' || message[parser->index] == 'f'){
            //We found a boolean (should be true or false) they are not quoted
            if (parser->state & VALUE && parser->state & CHARACTER){
               bool* boolVal = (bool*)malloc(sizeof(bool));
               returnStatus = parseJSONBoolean(parser, message, size, boolVal);
               if (!returnStatus){
                  elements[index] = boolVal;
                  types[index] = BOOLEAN;
                  index++;
                  
                  parser->state &= CLEAR_STATE;
                  parser->state |= (COMMA | CLOSE_BRACKET);
               }
               else {
                  return returnStatus;
               }
            }
            else {
               PUSH_ERROR(parser, JSON_UNEXPECTED_BOOLEAN, -1);
               json_errno = JSON_UNEXPECTED_BOOLEAN;
               return JSON_UNEXPECTED_BOOLEAN;
            }
         }
         else {
            PUSH_ERROR(parser, JSON_UNEXPECTED_CHARACTER, -1);
            json_errno = JSON_UNEXPECTED_CHARACTER;
            return JSON_UNEXPECTED_CHARACTER;
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
               return returnStatus;
            }
         }
         else {
            PUSH_ERROR(parser, JSON_UNEXPECTED_NUMBER, -1);
            json_errno = JSON_UNEXPECTED_NUMBER;
            return JSON_UNEXPECTED_NUMBER;
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
                  return returnStatus;
               }
            }
            else {
               PUSH_ERROR(parser, JSON_UNEXPECTED_STRING, -1);
               json_errno = JSON_UNEXPECTED_STRING;
               return JSON_UNEXPECTED_STRING;
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
                  return returnStatus;
               }
            }
            else {
               PUSH_ERROR(parser, JSON_UNEXPECTED_OBJECT, -1);
               json_errno = JSON_UNEXPECTED_OBJECT;
               return JSON_UNEXPECTED_OBJECT;
            }
         }
         else if (message[parser->index] == '['){
            //Found the beginning of an array
            if ((parser->state & VALUE) && (parser->state & OPEN_BRACKET)){
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
                  return returnStatus;
               }
            }
            else {
               PUSH_ERROR(parser, JSON_UNEXPECTED_ARRAY, -1);
               json_errno = JSON_UNEXPECTED_ARRAY;
               return JSON_UNEXPECTED_ARRAY;
            }
         }
         else if (message[parser->index] == ']'){
            if (parser->state & CLOSE_BRACKET){
               //Done here
               break;
            }
            else{
               PUSH_ERROR(parser, JSON_ARRAY_BRACKET_MISMATCH, -1);
               json_errno = JSON_ARRAY_BRACKET_MISMATCH;
               return JSON_ARRAY_BRACKET_MISMATCH;
            }
         }
         else if (message[parser->index] == ','){
            if (parser->state & COMMA){
               //Found seperator, we should expect another value
               parser->state &= CLEAR_STATE;
               parser->state |= (VALUE | QUOTE | CHARACTER | DIGIT | OPEN_PREN | OPEN_BRACKET);
            }
            else {
               PUSH_ERROR(parser, JSON_UNEXPECTED_COMMA, -1);
               json_errno = JSON_UNEXPECTED_COMMA;
               return JSON_UNEXPECTED_COMMA;
            }
         }
         else if (message[parser->index] == '}'){
            PUSH_ERROR(parser, JSON_OBJECT_BRACKET_MISMATCH, -1);
            json_errno = JSON_OBJECT_BRACKET_MISMATCH;
            return JSON_OBJECT_BRACKET_MISMATCH;
         }
         else {
            PUSH_ERROR(parser, JSON_UNEXPECTED_CHARACTER, -1);
            json_errno = JSON_UNEXPECTED_CHARACTER;
            return JSON_UNEXPECTED_CHARACTER;
         }
      }
      
      if (index >= arraySize){
         arraySize *= 2;
         elements = (void**) realloc(elements, sizeof(void*) * (arraySize + 1));
         types = (JSONType_t*) realloc(types, sizeof(JSONType_t) * (arraySize + 1));
         
         if (!elements || ! types){
            PUSH_ERROR(parser, JSON_MALLOC_FAIL, errno);
            json_errno = JSON_MALLOC_FAIL;
            return JSON_MALLOC_FAIL;
         }
      }
      
      parser->index++;
   }
   
   if (parser->index >= size){
      PUSH_ERROR(parser, JSON_MESSAGE_INCOMPLETE, -1);
      json_errno = JSON_MESSAGE_INCOMPLETE;
      return JSON_MESSAGE_INCOMPLETE;
   }
   
   array = newJSONArray(elements, types, index);
   
   //Need to free the booleans, numbers, and strings
   for (int i = 0; i < array->length; i++){
      if (types[i] == NUMBER || types[i] == BOOLEAN || types[i] == STRING){
         free(elements[i]);
      }
   }
   
   free(elements);
   free(types);
   
   *result= array;
   return JSON_SUCCESS;
}

/**
 * This helper function will parse out an key for a key:value pair and 
 * push it onto the key stack of the parser. 
 * 
 * @param parser - The parser object that is keeping track of this specific document
 * @param message - The JSON message 
 * @param size - The length of the message
 * @return JSON_SUCCESS if the key was parsed correctly, error otherwise (see stack trace)
 */
static JSONError_t  parseJSONKey(JSONParser_t* parser, const char* message, int size){
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
               return JSON_INVALID_KEY;
         }
      }
      else{
         temp[tempIndex++] = message[parser->index];
      }
      
      parser->index++;
   }
   
   if (parser->index >= size){
      PUSH_ERROR(parser, JSON_MESSAGE_INCOMPLETE, -1);
      json_errno = JSON_MESSAGE_INCOMPLETE;
      return JSON_MESSAGE_INCOMPLETE;
   }
   
   temp[tempIndex] = '\0';
   int tempSize = strlen(temp);
   //Create a string in the free store memory to hold the key
   char* key = (char*)calloc((tempSize + 1), sizeof(char));
   if (!key){
      PUSH_ERROR(parser, JSON_MALLOC_FAIL, errno);
      json_errno = JSON_MALLOC_FAIL;
      return JSON_MALLOC_FAIL;
   }
   
   //Copy the key and push it onto the stack
   strcpy(key, temp);
   if (parser->keyStackIndex < KEY_STACK_SIZE) { 
      parser->keyStack[parser->keyStackIndex++] = key;
   }
   else {
      return JSON_MESSAGE_TOO_LARGE;
   }
   
   return JSON_SUCCESS;
}

/**
 * Push an plain text error description onto info the parser object
 * for which the error occurred. 
 * 
 * @param parser - The parser object that was running when the error occurred
 * @param error - The error that occurred
 * @param currentFunction - The name of the function that is currently executing
 * @param currentFile - The name of the source file that is being executed
 * @param description - A breif, readable description of the problem
 */
static void pushError(JSONParser_t* parser, JSONError_t error, const char* currentFunction, const char* currentFile, int line, int errNo){
  if (!parser){
     return;
  }
  
  if (errNo > 0){
     sprintf(parser->tracebackString, "%s:%s():%d %s (%s) [state = 0x%x, lineNum = %d, index = %d]", 
             currentFile, currentFunction, line, json_strerror(error), strerror(errNo), 
             parser->state, parser->lineNumber, parser->index);
  }
  else {
     sprintf(parser->tracebackString, "%s:%s():%d %s [state = 0x%x, lineNum = %d, index = %d]", 
             currentFile, currentFunction, line, json_strerror(error), parser->state, 
             parser->lineNumber, parser->index);
  }
  
  parser->jsonError = error;
  parser->outsideError = errNo;
}
