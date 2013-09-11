#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#include "jsontools.h"

/*-----------------------------------------------------------------
 * Private Helper functions 
 *----------------------------------------------------------------*/
 
static JSONError_t writeJSONString(JSONKeyValue_t* pair, char** output, int depth, int* length);
static JSONError_t writeJSONNumber(JSONKeyValue_t* pair, char** output, int depth, int* length);
static JSONError_t writeJSONBoolean(JSONKeyValue_t* pair, char** output, int depth, int* length);
static JSONError_t writeJSONObject(JSONKeyValue_t* pair, char** output, int depth, int* length);
static JSONError_t writeJSONArray(JSONKeyValue_t* pair, char** output, int depth, int* length);
static JSONError_t writeJSONNull(JSONKeyValue_t* pair, char** output, int depth, int* length);
static char* indent(int depth);

/*-------------------------------------------------------------------
 * Implement global function
 *-----------------------------------------------------------------*/

/**
 * Converts a JSON document object to a JSON message string. 
 * 
 * @param document - The completed JSON document object that is to be converted
 * 
 * @param output - The string that will contain the contents of the JSON message
 * 
 * @param length - the actual length of the output string, or -1 if the max is too small
 * 
 * @return JSON_SUCCESS if everything converted properly, an error otherwise
 * 
 * @see JSONError_t
 */
JSONError_t documentToString(JSONKeyValue_t* document, char** output, int* length) {
   if (document == NULL) {
      json_errno = JSON_NULL_ARGUMENT;
      return JSON_NULL_ARGUMENT;
   }
   
      
   //We will need to calculate exactly how much room in memeory is need
   //to make this pair into a string value. 
   int valLen = 0;
   int otherStuff = 10;    //Other string formatting stuff
   int strLen = 0;
   
   char* values[document->length];
   int index = 0;
   
   JSONError_t status;
   JSONKeyValue_t* currentElement = NULL;
   if (document->type == OBJECT) {
      currentElement = document->value->oVal;
   }
   else if (document->type == ARRAY){
      currentElement = document->value->aVal;
   }
   else {
      return JSON_INVALID_VALUE;
   }
   
   int tempLength = 0;
   
   
   while(currentElement != NULL){
      switch (currentElement->type){
         case STRING:
            status = writeJSONString(currentElement, &values[index++], 1, &tempLength);
            break;
            
         case NUMBER:
            status = writeJSONNumber(currentElement, &values[index++], 1, &tempLength);
            break;
            
         case BOOLEAN:
            status = writeJSONBoolean(currentElement, &values[index++], 1, &tempLength);
            break;
            
         case ARRAY:
            status = writeJSONArray(currentElement, &values[index++], 1, &tempLength);
            break;
            
         case OBJECT:
            status = writeJSONObject(currentElement, &values[index++], 1, &tempLength);
            break;
         
         case NIL:
            status = writeJSONNull(currentElement, &values[index++], 1, &tempLength);
            break;
            
         default:
            json_errno = JSON_INVALID_TYPE;
            return JSON_INVALID_TYPE;
      }
      
      if (status != JSON_SUCCESS){
         json_errno = status;
         return status;
      }
      
      currentElement = currentElement->next;
   }
   
   for (int i = 0; i < document->length; i++) {
      valLen += strlen(values[i]);
      otherStuff += 3;
   }
   
   *output = (char*) malloc(sizeof(char) * (valLen + otherStuff));
   if (document->type == OBJECT) {
      sprintf(*output, "{\n");
   }
   else if (document->type == ARRAY){
      sprintf(*output, "[\n");
   }
   
   for (int i = 0; i < document->length - 1; i++){
      strcat(*output, values[i]);
      strcat(*output, ",\n");
   }
   
   if (document->length > 0){
      strcat(*output, values[document->length - 1]);
      strcat(*output, "\n");
   }
   
   if (document->type == OBJECT){
      strcat(*output, "}\n");
   }
   else if (document->type == ARRAY){
      strcat(*output, "]\n");
   }
   
   strLen = strlen(*output);
   *output = (char*) realloc(*output, strLen + 1);
   *length = strLen;
   
   //Free Memory
   for (int i = 0; i < document->length; i++){
         free(values[i]);
   }
   
   return JSON_SUCCESS;
}

/*-------------------------------------------------------------------
 * Implement private helper functions 
 *-----------------------------------------------------------------*/

/**
 * This helper function will convert a JSON pair that represents a string
 * a JSON message. This message can then be added to a larger message and 
 * passed to another JSON parser to be read in. 
 * 
 * @param pair - A JSON pair that represents a string 
 * @param output - The key and string value converted into a JSON message
 * @param depth - The indentation level of the message
 * @param length - The size of the JSON message after it has been converted, 
 * @return JSON_SUCCESS if the pair was converted into a JSON message, error otherwise
 */
static JSONError_t writeJSONString(JSONKeyValue_t* pair, char** output, int depth, int* length){
   if (pair){
      if (pair->type != STRING){
         json_errno = JSON_INVALID_ARGUMENT;
         return JSON_INVALID_ARGUMENT;
      }
      
      //We will need to calculate exactly how much room in memeory is need
      //to make this pair into a string value. 
      int keyLen = 0;
      int valLen = 0;
      int otherStuff = 10 + (2 * depth);    //Other string formatting stuff
      int strLen = 0;
      char* ind = indent(depth); //The indent string (this is dynamic so we need to free it)
      
      if (pair->key && pair->value && pair->value->sVal){
         keyLen = strlen(pair->key);
         valLen = strlen(pair->value->sVal);
         *output = (char*) calloc((keyLen + valLen + otherStuff), sizeof(char));
         strLen = sprintf(*output, "%s\"%s\" : \"%s\"", ind, pair->key, pair->value->sVal);
      }
      else if (pair->value && pair->value->sVal){
         valLen = strlen(pair->value->sVal);
         *output = (char*) calloc((valLen + otherStuff), sizeof(char));
         strLen = sprintf(*output, "%s\"%s\"", ind, pair->value->sVal);
      }
      else{
         json_errno = JSON_NULL_VALUE;
         return JSON_NULL_VALUE;
      }
      
      free(ind);
      
      *length = strLen;
      
   }
   else{
      json_errno = JSON_NULL_ARGUMENT;
      return JSON_NULL_ARGUMENT;
   }
   
   return JSON_SUCCESS;
}

/**
 * This helper function will write out a number object as a JSON message.
 * If the number has no fractioanl portion, it will be written like an integer.
 * If the number has a fractional part, it will be written as a double. 
 * 
 * @param pair - A JSON pair that represents a string 
 * @param output - The key and string value converted into a JSON message
 * @param depth - The indentation level of the message
 * @param length - The size of the JSON message after it has been converted, 
 * @return JSON_SUCCESS if the pair was converted into a JSON message, error otherwise
 */
static JSONError_t writeJSONNumber(JSONKeyValue_t* pair, char** output, int depth, int* length){
   if (pair){
      if (pair->type != NUMBER){
         json_errno = JSON_INVALID_ARGUMENT;
         return JSON_INVALID_ARGUMENT;
      }
      
      if (!pair->value){
         json_errno = JSON_NULL_VALUE;
         return JSON_NULL_VALUE;
      }
      
      //Need to see if the number has a fractional part, if it does
      //we print a float, if it does not, we print a integer. 
      long long rawNum = (long long)pair->value->nVal;
      bool hasFractional;
      if (fabs(pair->value->nVal) - llabs(rawNum) > 0.0){
         hasFractional = true;
      }
      else {
         hasFractional = false;
      }
      
      //We will need to calculate exactly how much room in memeory is need
      //to make this pair into a string value. 
      int keyLen = 0;
      int valLen = 50;     //Too much time to find exactly how many digits the number has, safe at 50
      int otherStuff = 10 + (2 * depth);    //Other string formatting stuff
      int strLen = 0;
      char* ind = indent(depth); //The indent string (this is dynamic so we need to free it)
      
      if (pair->key && pair->value){
         keyLen = strlen(pair->key);
         *output = (char*) calloc((keyLen + valLen + otherStuff), sizeof(char));
         if (hasFractional){
            strLen = sprintf(*output, "%s\"%s\" : %f", ind, pair->key, pair->value->nVal);
         }
         else {
            strLen = sprintf(*output, "%s\"%s\" : %lld", ind, pair->key, rawNum);
         }
         
      }
      else if (pair->value){
         *output = (char*) calloc((valLen + otherStuff), sizeof(char));
         if (hasFractional){
            strLen = sprintf(*output, "%s%f", ind,  pair->value->nVal);
         }
         else {
            strLen = sprintf(*output, "%s%lld", ind, rawNum);
         }
      }
      else {
          json_errno = JSON_NULL_VALUE;
         return JSON_NULL_VALUE;
      }
      
      if (strLen < 0){
         json_errno = JSON_INTERNAL_FAILURE;
         return JSON_INTERNAL_FAILURE;
      }
      
      *length = strLen;
      free(ind);
   }
   else {
      json_errno = JSON_NULL_ARGUMENT;
      return JSON_NULL_ARGUMENT; 
   }
   
   return JSON_SUCCESS;
}

/**
 * This helper function will write out a boolean object as a JSON message. 
 * The value will be an unquoted true or false value. 
 * 
 * @param pair - A JSON pair that represents a string 
 * @param output - The key and string value converted into a JSON message
 * @param depth - The indentation level of the message
 * @param length - The size of the JSON message after it has been converted, 
 * @return JSON_SUCCESS if the pair was converted into a JSON message, error otherwise
 */
static JSONError_t writeJSONBoolean(JSONKeyValue_t* pair, char** output, int depth, int* length){
   if (pair){
      if (pair->type != BOOLEAN){
         json_errno = JSON_INVALID_ARGUMENT;
         return JSON_INVALID_ARGUMENT;
      }
      
      char* t = "true";
      char* f = "false";
      
      //We will need to calculate exactly how much room in memeory is need
      //to make this pair into a string value. 
      int keyLen = 0;
      int valLen = 0; 
      int otherStuff = 10 + (2 * depth);    //Other string formatting stuff
      int strLen = 0;
      char* ind = indent(depth); //The indent string (this is dynamic so we need to free it)
      
      if (pair->key && pair->value){
         keyLen = strlen(pair->key);
         valLen = ((pair->value->bVal) ? strlen(t) : strlen(f));
         *output = (char*) calloc((keyLen + valLen + otherStuff), sizeof(char));
         strLen = sprintf(*output, "%s\"%s\" : %s", ind, pair->key, ((pair->value->bVal) ? t : f));
      }
      else if (pair->value){
         valLen = ((pair->value->bVal) ? strlen(t) : strlen(f));
         *output = (char*) calloc((valLen + otherStuff), sizeof(char));
         strLen = sprintf(*output, "%s%s", ind, ((pair->value->bVal) ? t : f));
      }
      else {
         return JSON_NULL_VALUE;
      }
      
      *length = strLen;
      free(ind);
   }
   else{
      json_errno = JSON_INVALID_ARGUMENT;
      return JSON_INVALID_ARGUMENT;
   }
   return JSON_SUCCESS;
}

/**
 * This helper function will write out a JSON object type to a JSON message.
 * Since objects contain unordered key:value pairs, and those values can be
 * any valid JSON value including other objects, this function will call
 * the others recurrsivly if necessary. 
 * 
 * @param pair - A JSON pair that represents a string 
 * @param output - The key and string value converted into a JSON message
 * @param depth - The indentation level of the message
 * @param length - The size of the JSON message after it has been converted, 
 * @return JSON_SUCCESS if the pair was converted into a JSON message, error otherwise
 */
static JSONError_t writeJSONObject(JSONKeyValue_t* pair, char** output, int depth, int* length){
   if (pair){
      if (pair->type != OBJECT) {
         json_errno = JSON_INVALID_ARGUMENT;
         return JSON_INVALID_ARGUMENT;
      }
      
      if (!pair->value || !pair->value->oVal) {
         json_errno = JSON_NULL_VALUE;
         return JSON_NULL_VALUE;
      }
      
      //Find out how many children we have 
      JSONKeyValue_t* current = pair->value->oVal;
      int count = 1;
      while(current && current->next) {
         count++;
         current = current->next;
      }
      
      char* values[count];    //hold strings for each child
      
      //We will need to calculate exactly how much room in memeory is need
      //to make this pair into a string value. 
      int keyLen = 0;
      int valLen = 0; 
      int otherStuff = 10 + (4 * depth);    //Other string formatting stuff
      int strLen = 0;
      char* ind = indent(depth); //The indent string (this is dynamic so we need to free it)
      
      if (pair->key) {
         keyLen = strlen(pair->key);
      }
      
      //Loop through all of the objects children and produce string values for them
      int index = 0;
      JSONError_t status;
      current = pair->value->oVal;
      int tempLength = 0;
      while(current != NULL) {
         switch (current->type) {
            case STRING:
               status = writeJSONString(current, &values[index++], depth + 1, &tempLength);
               break;
               
            case NUMBER:
               status = writeJSONNumber(current, &values[index++], depth + 1, &tempLength);
               break;
               
            case BOOLEAN:
               status = writeJSONBoolean(current, &values[index++], depth + 1, &tempLength);
               break;
               
            case ARRAY:
               status = writeJSONArray(current, &values[index++], depth + 1, &tempLength);
               break;
               
            case OBJECT:
               status = writeJSONObject(current, &values[index++], depth + 1, &tempLength);
               break;
            
            case NIL:
               status = writeJSONNull(current, &values[index++], depth + 1, &tempLength);
               break;
               
            default:
               json_errno = JSON_INVALID_TYPE;
               return JSON_INVALID_TYPE;
         }
         
         if (status != JSON_SUCCESS){
            return status;
         }
         
         current = current->next;
      }
      
      //Build string based on parsed values
      for (int i = 0; i < count; i++){
         valLen += strlen(values[i]);
         otherStuff += 3;     //null char + comma + newline
      }
      
      *output = (char*) calloc((keyLen + valLen + otherStuff), sizeof(char));
      
      if (pair->key){
         sprintf(*output, "%s\"%s\" : {\n", ind, pair->key);
      }
      else {
         sprintf(*output, "%s{\n", ind);
      }
         
      for (int i = 0; i < count - 1; i++){
         strcat(*output, values[i]);
         strcat(*output, ",\n");
      }
      
      strcat(*output, values[count - 1]);
      strcat(*output, "\n");
      strcat(*output, ind);
      strcat(*output, "}");
      
      strLen = strlen(*output);
      *output = (char*) realloc(*output, strLen + 1);
      *length = strLen;

      //Free Memory
      free(ind);
      for (int i = 0; i < count; i++){
         free(values[i]);
      }
      
   }
   else{
      json_errno = JSON_NULL_ARGUMENT;
      return JSON_NULL_ARGUMENT;
   }
   
   return JSON_SUCCESS;
}

/**
 * This helper function will write out a JSON array as a JSON message. This
 * function will call the other helper functions recurrsivly. 
 * 
 * @param pair - A JSON pair that represents a string 
 * @param output - The key and string value converted into a JSON message
 * @param depth - The indentation level of the message
 * @param length - The size of the JSON message after it has been converted, 
 * @return JSON_SUCCESS if the pair was converted into a JSON message, error otherwise
 */
static JSONError_t writeJSONArray(JSONKeyValue_t* pair, char** output, int depth, int* length){
   if (pair){
      if (pair->type != ARRAY){
         json_errno = JSON_INVALID_ARGUMENT;
         return JSON_INVALID_ARGUMENT;
      }
      
      //We will need to calculate exactly how much room in memeory is need
      //to make this pair into a string value. 
      int keyLen = 0;
      int valLen = 0; 
      int otherStuff = 10 + (4 * depth);    //Other string formatting stuff
      int strLen = 0;
      char* ind = indent(depth); //The indent string (this is dynamic so we need to free it)
      
      char* values[pair->length];   //Hold the string values for the elements of the array
      
      if (pair->key){
         keyLen = strlen(pair->key);
      }
      
      JSONKeyValue_t* current = pair->value->aVal;
      JSONError_t status;
      int index = 0;
      int tempLength = 0;
      while(current != NULL){
         switch (current->type){
            case STRING:
               status = writeJSONString(current, &values[index++], depth + 1, &tempLength);
               break;
            
            case NUMBER:
               status = writeJSONNumber(current, &values[index++], depth + 1, &tempLength);
               break;
               
            case BOOLEAN:
               status = writeJSONBoolean(current, &values[index++], depth + 1, &tempLength);
               break;
               
            case ARRAY:
               status = writeJSONArray(current, &values[index++], depth + 1, &tempLength);
               break;
               
            case OBJECT:
               status = writeJSONObject(current, &values[index++], depth + 1, &tempLength);
               break;
               
            case NIL:
               status = writeJSONNull(current, &values[index++], depth + 1, &tempLength);
               break;
               
            default:
               json_errno = JSON_INVALID_TYPE;
               return JSON_INVALID_TYPE;
               break;
            
         }
         
         if (status != JSON_SUCCESS){
            return status;
         }
         
         current = current->next;
      }
      
      //Build array string
      
      //find how much memory to use
      
      for (int i = 0; i < pair->length; i++){
         valLen += (strlen(values[i]));
         otherStuff += 3;  //null char, comma, newline
      }
      
      *output = (char*) calloc((keyLen + valLen + otherStuff), sizeof(char));
      
      if (pair->key){
         sprintf(*output, "%s\"%s\" : [\n", ind, pair->key);
      }
      else {
         sprintf(*output, "%s[\n", ind);
      }
      
      for (int i = 0; i < pair->length - 1; i++){
         strcat(*output, values[i]);
         strcat(*output, ",\n");
      }
      
      if (pair->length > 0){
         strcat(*output, values[pair->length - 1]);
      }
      
      strcat(*output, "\n");
      strcat(*output, ind);
      strcat(*output, "]");
      
      strLen = strlen(*output);
      *output = (char*) realloc(*output, strLen + 1);
      *length = strLen;
      
      //Free Memory
      free(ind);
      for (int i = 0; i < pair->length; i++){
         free(values[i]);
      }
      
   }
   else {
      json_errno = JSON_NULL_ARGUMENT;
      return JSON_NULL_ARGUMENT;
   }
   
   return JSON_SUCCESS;
}

/**
 * This helper function will write out a JSON null value as a JSON message.
 * A JSON null value is simply the unquoted word null, with no key. 
 * 
 * @param pair - A JSON pair that represents a string 
 * @param output - The key and string value converted into a JSON message
 * @param depth - The indentation level of the message
 * @param length - The size of the JSON message after it has been converted, 
 * @return JSON_SUCCESS if the pair was converted into a JSON message, error otherwise
 */
static JSONError_t writeJSONNull(JSONKeyValue_t* pair, char** output, int depth, int* length) {
   if (pair) {
      if (pair->type != NIL) {
         json_errno = JSON_INVALID_ARGUMENT;
         return JSON_INVALID_ARGUMENT;
      }
      
      //We will need to calculate exactly how much room in memeory is need
      //to make this pair into a string value. 
      int valLen = 5 + (pair->key ? strlen(pair->key) : 0);   //The word null plus the key
      int otherStuff = 3 + (2 * depth);    //Other string formatting stuff 3 = 2 for quotes, 1 for colon
      int strLen = 0;
      char* ind = indent(depth); //The indent string (this is dynamic so we need to free it)
      
      *output = (char*) calloc((valLen + otherStuff), sizeof(char));
      
      if (pair->key){
         strLen = sprintf(*output, "%s\"%s\":%s", ind, pair->key, "null");
      }
      else {
         strLen = sprintf(*output, "%s%s", ind, "null");
      }
      
      *length = strLen;
      free(ind);
   }
   else {
      json_errno = JSON_NULL_ARGUMENT;
      return JSON_NULL_ARGUMENT;
   }
   
   return JSON_SUCCESS;
}

/**
 * This will create the approperate amount of spaces for the indent depth
 * of the message being generated. This string is created dynamicly so it
 * must be freed when it is no longer needed to avoid memory leaks. 
 */
static char* indent(int depth) {
   int length = (depth * 2) + 1;
   char* output = (char*) calloc(length, sizeof(char));
   for (int i = 0; i < depth; i++){
      strcat(output, "  ");
   }
   
   return output;
}
