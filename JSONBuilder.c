
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>

#include "JSONTools.h"

/*-------------------------------------------------------------------
 * Implement global functions 
 *-----------------------------------------------------------------*/


/**
 * Creates a new JSON string object. This can be attached as a value
 * to a key value pair. 
 * 
 * @param string - The string to make into a JSON string object
 * 
 * @return The JSON string value that can be added to a pair, or array
 */
JSONValue_t* newJSONString(char* string) {
   JSONValue_t* stringValue = (JSONValue_t*) malloc(sizeof(JSONValue_t));
   
   if (stringValue == NULL){
      return NULL;
   }
   
   memset(stringValue, 0, sizeof(JSONValue_t));
   
   int stringLength = strlen(string);
      
   int specialCharCount = 0;
   for (int i = 0; i < stringLength; i++){
      if (string[i] == '"' || 
          string[i] == '\\' ||
          string[i] == '\b' ||
          string[i] == '\f' ||
          string[i] == '\n' ||
          string[i] == '\r' ||
          string[i] == '\t'){
             
             specialCharCount++;
      }
   }
   
   char* newString = (char*) malloc(sizeof(char) * ((2 * specialCharCount) + stringLength) + 1);
   
   if(newString == NULL){
      return NULL;
   }
   
   /* 
    * If we have special characters in the string, we need to parse the
    * string and convert those characters to their JSON escaped equivilants
    * 
    *  \"
    *  \\
    *  \/   <-- this one is touchy, '/' is supposed to be esacped, but no one does this in practice. we just ignore it.
    *  \b
    *  \f
    *  \n
    *  \r
    *  \t
    */
   if (specialCharCount){
      int strIndex = 0;
      for (int i = 0; i < stringLength; i++){
         if (string[i] == '"'){
            newString[strIndex++] = '\\';
            newString[strIndex++] = '"';
         }
         else if (string[i] =='\\'){
            if (string[i+1] == '\\' ||
                string[i+1] == '/' ||
                string[i+1] == '"' ||
                string[i+1] == 'u' ||
                string[i+1] == 'b' ||
                string[i+1] == 'f' ||
                string[i+1] == 'n' ||
                string[i+1] == 'r' ||
                string[i+1] == 't') {
               //Don't re-escape an escaped sequence...
               newString[strIndex++] = string[i];    
            }
            else {
               newString[strIndex++] = '\\';
               newString[strIndex++] = '\\';
            }
         }
         else if (string[i] =='\b'){
            newString[strIndex++] = '\\';
            newString[strIndex++] = 'b';
         }
         else if (string[i] =='\f'){
            newString[strIndex++] = '\\';
            newString[strIndex++] = 'f';
         }
         else if (string[i] =='\n'){
            newString[strIndex++] = '\\';
            newString[strIndex++] = 'n';
         }
         else if (string[i] =='\r'){
            newString[strIndex++] = '\\';
            newString[strIndex++] = 'r';
         }
         else if (string[i] =='\t'){
            newString[strIndex++] = '\\';
            newString[strIndex++] = 't';
         }
         else{
            newString[strIndex++] = string[i];
         }
      }
   }
   else{
      strcpy(newString, string);
      //Just in case... set the last element to a null character
      newString[stringLength] = '\0';
   }
   
   stringValue->sVal = newString;
   
   return stringValue;
}

/**
 * Creates a new JSON number object. This can be attached as a value
 * to a key value pair.
 * 
 * @param number - The double percision number to make into a JSON number object
 * 
 * @return The JSON number value that can be added to a pair, or array
 */
JSONValue_t* newJSONNumber(double number) {
   JSONValue_t* newNumber = (JSONValue_t*) malloc(sizeof(JSONValue_t));
   
   if (newNumber == NULL) {
      PUSH_ERROR(INTERNAL_FAILURE, "Unable to allocate memeory for JSON number");
      return NULL;
   }
   
   memset(newNumber, 0, sizeof(JSONValue_t));
   
   newNumber->nVal = number;
   
   return newNumber;
}

/**
 * Creates a new JSON boolean object. This can be attached as a value
 * to a key value pair.
 * 
 * @param boolean - a true or false value to make into a JSON boolean object
 * 
 * @return The JSON boolean value that can be added to a pair, or array
 */
JSONValue_t* newJSONBoolean(bool boolean) {
   JSONValue_t* newBoolean = (JSONValue_t*) malloc(sizeof(JSONValue_t));
   
   if (newBoolean == NULL) {
      PUSH_ERROR(INTERNAL_FAILURE, "Unable to allocate memory for boolean object");
      return NULL;
   }
   
   memset(newBoolean, 0, sizeof(JSONValue_t));
   
   newBoolean->bVal = boolean;
   
   return newBoolean;
}

/**
 * Creates a new JSON object object. A JSON object can hold other 
 * key value pairs as an unordered list. 
 * 
 * @param pair - The JSON pair object to attach to this JSON object object. 
 * 
 * @return The JSON object value that can be added to a pair, or array
 */
JSONValue_t* newJSONObject(JSONKeyValue_t* pair) {
   JSONValue_t* newObject = (JSONValue_t*) malloc(sizeof(JSONValue_t));
   
   if (newObject == NULL){
      return NULL;
   }
   
   memset(newObject, 0, sizeof(JSONValue_t));
   
   newObject->oVal = (JSONKeyValue_t*)malloc(sizeof(JSONKeyValue_t));
   memcpy(newObject->oVal, pair, sizeof(JSONKeyValue_t));
   
   return newObject;
}

/**
 * Attaches the pair to the end of the object value. This is used to create
 * a linked list of key:value pairs that represent the object value. 
 * 
 * @param object - The JSON object object to attach the key value pair to
 * 
 * @param pair - The key value pair to attach to the object value
 * 
 * @return The JSON object that this pair was attached to. 
 */
JSONValue_t* addKeyValuePair(JSONValue_t* object, JSONKeyValue_t* pair){
   if (object && pair){
      JSONKeyValue_t* current = object->oVal;
      
      if (current == NULL){
         object->oVal = (JSONKeyValue_t*)malloc(sizeof(JSONKeyValue_t));
         if (object->oVal){
            memcpy(object->oVal, pair, sizeof(JSONKeyValue_t));
         }
         else {
            PUSH_ERROR(INTERNAL_FAILURE, "Unable to allocate memory for pair");
            return NULL;
         }
      }
      else {
         while (current->next != NULL){
            current = current->next;
         }
         
         current->next = (JSONKeyValue_t*)malloc(sizeof(JSONKeyValue_t));
         memcpy(current->next, pair, sizeof(JSONKeyValue_t));
      }
   }
   else {
      PUSH_ERROR(NULL_ARGUMENT, "The arguments were null");
      return NULL;
   }
   
   return object;
}

/**
 * Creates a new JSON array object. The contents of the the array are built 
 * into the JSON Array object and returned as a value object.
 * 
 * @param key - The unique identifier for the key:value pair
 * 
 * @param array - The actual values that will be put into the array
 * 
 * @param types - The corresponding types for the values
 * 
 * @param length - The number of values being put into the JSON array
 * 
 * @return The JSON key:value pair with the values inside of the array
 */
JSONKeyValue_t* newJSONArray(char* key, void* array[], JSONType_t types[], int length) {
   JSONKeyValue_t* newArray = (JSONKeyValue_t*) malloc(sizeof(JSONKeyValue_t));
   JSONValue_t* arrayValue = (JSONValue_t*) malloc(sizeof(JSONValue_t));
   
   if (newArray == NULL || arrayValue == NULL){
      return NULL;
   }
   
   memset(newArray, 0, sizeof(JSONKeyValue_t));
   memset(arrayValue, 0, sizeof(JSONValue_t));
   
   newArray->value = arrayValue;
   JSONKeyValue_t* currentElement = newArray->value->aVal;
   JSONKeyValue_t* element = NULL;
   JSONValue_t* value = NULL;
   for (int i = 0; i < length; i++){
      switch (types[i]){
         case NUMBER:
            value = newJSONNumber(*((double*)array[i]));
            element = newJSONPair(types[i], NULL, value);
            break;
            
         case STRING:
            value = newJSONString((char*)array[i]);
            element = newJSONPair(types[i], NULL, value);
            break;
            
         case BOOLEAN:
            value = newJSONBoolean(*((bool*)array[i]));
            element = newJSONPair(types[i], NULL, value);
            break;
            
         case ARRAY:
            element = (JSONKeyValue_t*)malloc(sizeof(JSONKeyValue_t));
            if (element){
               memcpy(element, ((JSONKeyValue_t*)array[i]), sizeof(JSONKeyValue_t));
            }
            else {
               PUSH_ERROR(INTERNAL_FAILURE, "Unable to allocate memory for array object");
               return NULL;
            }
            break;
            
         case OBJECT:
            value = ((JSONValue_t*)array[i]);
            element = newJSONPair(types[i], NULL, value);
            break;
            
         case NIL:
            value = NULL;
            element = newJSONPair(types[i], NULL, NULL);
            break;
            
         default:
            //Error here
            break;
            
      }
      
      if (currentElement == NULL){
         newArray->value->aVal = element;
         currentElement = newArray->value->aVal;
      }
      else {
         currentElement->next = element;
         currentElement = currentElement->next;
      }
   }
   
   if (key){
      int keyLength = strlen(key);
      newArray->key = (char*) malloc(sizeof(char) * (keyLength + 1));
      if (newArray->key == NULL){
         return NULL;
      }
      
      strcpy(newArray->key, key);
   }
   
   newArray->length = length;
   newArray->type = ARRAY;
   
   return newArray;
}

/**
 * Creates a new JSON key value pair. The key is a unique identifier 
 * that can be used to lookup the pair in a collection of pairs. This key 
 * need not be unique, but must uniquely identify the data expected under it
 * 
 * @param type - The type of data contained in the value
 * 
 * @param key - The unique identifier for the key:value pair
 * 
 * @param value - A previously creatd JSON value that matches the type
 */
JSONKeyValue_t* newJSONPair(JSONType_t type, char* key, JSONValue_t* value) {
   JSONKeyValue_t* newPair = (JSONKeyValue_t*) malloc(sizeof(JSONKeyValue_t));
   
   if (newPair == NULL){
      return NULL;
   }
   
   memset(newPair, 0, sizeof(JSONKeyValue_t));
   
    if (key){
      int keyLength = strlen(key);
      newPair->key = (char*) malloc(sizeof(char) * (keyLength + 1));
      if (newPair->key == NULL){
         return NULL;
      }
      
      strcpy(newPair->key, key);
   }
   
   int count = 0;
   if (type == OBJECT){
      JSONKeyValue_t* current = value->oVal;
      while(current != NULL){
         count++;
         current = current->next;
      }
   }
   else {
      count = 1;
   }
   
   newPair->type = type;
   newPair->value = value;
   newPair->length = count;
   
   return newPair;
}
