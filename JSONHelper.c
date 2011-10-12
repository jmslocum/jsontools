#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdbool.h>

#include "JSONTools.h"

/**
 * Helper function that converts an integer (unicode character) to
 * a sequence of unicode bytes. The bytes can then be written into
 * a string and displayed on a UTF-8 compatable terminal or file 
 * reader. 
 * 
 * @param character - The unicode character (up to 16 bits wide is what JSON allows)
 * 
 * @param utfBytes - an array of size 6, this array will be filled with UTF-8 bytes
 * 
 * @return - Number of bytes in utfBytes array.
 * 
 * NOTE - Always pass a char array of size 6 into the functon to avoid segfaulting
 */
static int convertToUTF8(unsigned int character, char utfBytes[]);

/*------------------------------------------------------------------
 * Implement global functions
 *-----------------------------------------------------------------*/

/**
 * Looks to see if there is a child for this parent. This is useful if
 * you want to test a key before you attempt to reterive it. 
 * 
 * @param parent - The parent key:value pair whos value is another key:value pair
 * 
 * @param key - The key string of the key:value pair you want to reterive. 
 * 
 * @return - The true if the key exists, false otherwise
 */
bool hasChildPair(JSONKeyValue_t* parent, const char* key){
   if (!parent || !key){
      return false;
   }
   
   if (parent->type != OBJECT){
      return false;
   }
   
   JSONKeyValue_t* current = parent->value->oVal;
   while(current != NULL){
      if (current->type != NIL){
         if (strcmp(current->key, key) == 0){
            return true;
         }
      }
      
      current = current->next;
   }
   
   return false;
}

/**
 * Gets the child element for this JSON object. This is useful to reterive
 * nested key:value pairs from JSON object types. If this key value pair 
 * does not represent an object type (has no children), or if none of
 * children have the requested key then NULL is returned. 
 * 
 * @param parent - The parent key:value pair whos value is another key:value pair
 * 
 * @param key - The key string of the key:value pair you want to reterive. 
 * 
 * @return - The JSONKeyValue_t* object that matches the request, or NULL
 */
JSONKeyValue_t* getChildPair(JSONKeyValue_t* parent, const char* key){
   if (!parent || !key){
      return NULL;
   }
   
   if (parent->type != OBJECT){
      return NULL;
   }
   
   JSONKeyValue_t* current = parent->value->oVal;
   while(current != NULL){
      if (current->type != NIL){
         if (strcmp(current->key, key) == 0){
            return current;
         }
      }
      
      current = current->next;
   }
   
   return NULL;
}

/**
 * Gets the child elements for this JSON object. This is useful to reterive
 * nested key:value pairs from JSON object types, when the objects 
 * have the same key. If this key value pair does not represent an 
 * object type (has no children), or if none of the children have
 * the requested key then NULL is returned. 
 * 
 * @param parent - The parent key:value pair whos value is another key:value pair
 * 
 * @param key - The key string of the key:value pair you want to reterive. 
 * 
 * @param length - The number of key:value pairs found
 * 
 * @return - The JSONKeyValue_t* object that matches the request, or NULL
 */
JSONKeyValue_t** getChildPairs(JSONKeyValue_t* parent, const char* key, int* length){
   if (!parent || !key || !length){
      return NULL;
   }
   
   if (parent->type != OBJECT){
      *length = -1;
      return NULL;
   }
   
   //Count how many pairs match the key requested
   int count = 0;
   JSONKeyValue_t* current = parent->value->oVal;
   
   while(current != NULL){
      if (current->type != NIL){
         if (strcmp(current->key, key) == 0){
            count++;
         }
      }
   }
   
   if (count <= 0){
      *length = 0;
      return NULL;
   }
   
   //Create an array of the correct size and start adding those pairs to the array
   JSONKeyValue_t** pairs = (JSONKeyValue_t**) malloc(sizeof(JSONKeyValue_t*) * count);
   *length = count;
   int index = 0;
   current = parent->value->oVal;
   while(current != NULL){
      if (current->type != NIL){
         if (strcmp(current->key, key) == 0){
            pairs[index++] = current;
         }
      }
   }
   
   return pairs;
}

/**
 * Gets the child elements for this JSON object. This is useful to reterive
 * nested key:value pairs from JSON object types, regardless of the key.
 * The order of the elements will be preserved, and nulls will be inserted
 * where null JSON values are found. If this key value pair does not represent an 
 * object type (has no children), or if none of the children have
 * the requested key then NULL is returned.
 * 
 * @param parent - The parent key:value pair whos value is another key:value pair
 * 
 * @param length - The number of key:value pairs found
 * 
 * @return - The JSONKeyValue_t* object that matches the request, or NULL
 */
JSONKeyValue_t** getAllChildPairs(JSONKeyValue_t* parent, int* length){
   if (!parent || !length){
      return NULL;
   }
   
   if (parent->type != OBJECT){
      *length = -1;
      return NULL;
   }
   
   //Loop through the linked list of JSON values and add them to the array
   int index = 0;
   JSONKeyValue_t* current = parent->value->oVal;
   JSONKeyValue_t** pairs = (JSONKeyValue_t**) malloc(sizeof(JSONKeyValue_t*) * parent->length);
   current = parent->value->oVal;
   while(current != NULL){
      if (current->type == NIL) {
         pairs[index] = NULL;
      }
      else {
         pairs[index] = current;
      }
      
      current = current->next;
      index++;
   }
   
   *length = parent->length;
   return pairs;
}

/**
 * Removes the first key:value pair from the object that matches the 
 * specified key. This will move the other pairs so that the rest of the
 * object is maintained. 
 * 
 * @param parent - A JSON object pair
 * @param key - The key of the key:value pair to remove
 * @return SUCCESS if the pair was removed, NO_MATCHING_PAIR if the pair could not be found, error otherwise
 */
JSONError_t removeChildPair(JSONKeyValue_t* parent, const char* key){
   if (!parent || !key){
      return JSON_NULL_ARGUMENT;
   }
   
   if (parent->type != OBJECT){
      return JSON_INVALID_ARGUMENT;
   }
   
   //Start looking through the list for the pair with the requested key
   //Make sure we ignore null values because they don't have keys. 
   JSONKeyValue_t* previous = parent->value->oVal;
   JSONKeyValue_t* current = parent->value->oVal;
   JSONKeyValue_t* next = current->next;
   while(current != NULL){
      if (current->type != NIL){
         if (strcmp(current->key, key) == 0){
            previous->next = next;
            disposeOfPair(current);
            parent->length--;
            return JSON_SUCCESS;
         }
      }
      
      previous = current;
      current = current->next;
      if (current != NULL) {
         next = current->next;
      }
      else {
         next = NULL;
      }
   }
   
   return JSON_NO_MATCHING_PAIR;
}

/**
 * Removes the first key:value pair from the object that matches the 
 * specified key. This will move the other pairs so that the rest of the
 * object is maintained. 
 * 
 * @param parent - A JSON object pair
 * @param key - The key of the key:value pair to remove
 * @return SUCCESS if the pair was removed, NO_MATCHING_PAIR if the pair could not be found, error otherwise
 */
JSONError_t removeChildPairs(JSONKeyValue_t* parent, const char* key){
   if (!parent || !key){
      return JSON_NULL_ARGUMENT;
   }
   
   if (parent->type != OBJECT){
      return JSON_INVALID_ARGUMENT;
   }
   
   //Start looking through the list for the pair with the requested key
   //Make sure we ignore null values because they don't have keys. 
   JSONKeyValue_t* previous = parent->value->oVal;
   JSONKeyValue_t* current = parent->value->oVal;
   JSONKeyValue_t* next = current->next;
   int found = 0;
   while(current != NULL){
      if (current->type != NIL){
         if (strcmp(current->key, key) == 0){
            previous->next = next;
            disposeOfPair(current);
            //Now that we have disposed of the element, we need to point to a safe one
            current = previous;
            parent->length--;
         }
      }
      
      previous = current;
      current = current->next;
      if (current != NULL) {
         next = current->next;
      }
      else {
         next = NULL;
      }
   }
   
   if (found){
      return JSON_SUCCESS;
   }
   
   return JSON_NO_MATCHING_PAIR;
}

/**
 * Gets an array of the types if the pair represents an array, Since 
 * a valid JSON array can contain values of different types, the function
 * will return an array of void pointers containing the objects, and an
 * array of types for those objects in the same order. 
 * 
 * @param pair - The JSONKeyValue pair object that contains the array
 * 
 * @param values - an array of void pointers that will be populated with the objects
 * 
 * @param types - the types of each of those objects contained in values
 * 
 * @return - SUCCESS, or an error 
 */
JSONError_t getArray(JSONKeyValue_t* pair, void* values[], JSONType_t types[]){
   if (!pair || !values || !types){
      return JSON_NULL_ARGUMENT;
   }
   
   if (pair->type != ARRAY){
      return JSON_INVALID_ARGUMENT;
   }
   
   JSONKeyValue_t* current = pair->value->aVal;
   int index = 0;
   while(current != NULL){
      switch(current->type){
         case STRING: {
            types[index] = STRING;
            values[index] = current->value->sVal;
            break;
         }
         
         case NUMBER: {
            types[index] = NUMBER;
            double* newNum = (double*)malloc(sizeof(double));
            *newNum = current->value->nVal;
            values[index] = newNum;
            break;
         }
         
         case BOOLEAN: {
            types[index] = BOOLEAN;
            bool* newBool = (bool*)malloc(sizeof(bool));
            *newBool = current->value->bVal;
            values[index] = newBool;
            break;
         }
            
         case NIL: {
            types[index] = NIL;
            values[index] = NULL;
            break;
         }
            
         case OBJECT: {
            types[index] = OBJECT;
            values[index] = current->value->oVal;
            break;
         }
            
         case ARRAY: {
            types[index] = ARRAY;
            values[index] = current->value->aVal;
            break;
         }
         
         default: {
            return JSON_INVALID_TYPE;
            break;
         }
      }
      
      index++;
      current = current->next;
   }
   
   return JSON_SUCCESS;
}

/**
 * Gets the string contents for a key:value pair that holds a string value
 * 
 * @param pair - The JSONKeyValue_t* that you want to get string from
 * 
 * @param value - a pointer to a char* that will hold the string value
 * 
 * @return - SUCCESS if everything ran correctly, error otherwise. 
 * 
 */
JSONError_t getString(JSONKeyValue_t* pair, char** value){
   if (!pair || !value){
      return JSON_NULL_ARGUMENT;
   }
   
   if (pair->type != STRING){
      return JSON_INVALID_ARGUMENT;
   }
   
   *value = pair->value->sVal;
   
   return JSON_SUCCESS;
}

/**
 * Gets the number contents for a key:value pair that holds a number value
 * 
 * @param pair - The JSONKeyValue_t* that you want to get number from
 * 
 * @param value - a double* that will hold the number value
 * 
 * @return - SUCCESS if everything ran correctly, error otherwise. 
 * 
 */
JSONError_t getNumber(JSONKeyValue_t* pair, double* value){
   if (!pair || !value){
      return JSON_NULL_ARGUMENT;
   }
   
   if (pair->type != NUMBER){
      return JSON_INVALID_ARGUMENT;
   }
   
   *value = pair->value->nVal;
   
   return JSON_SUCCESS;
}

/**
 * Gets the boolean contents for a key:value pair that holds a boolean value
 * 
 * @param pair - The JSONKeyValue_t* that you want to get boolean from
 * 
 * @param value - a bool* that will hold the boolean value
 * 
 * @return - SUCCESS if everything ran correctly, error otherwise. 
 * 
 */
JSONError_t getBoolean(JSONKeyValue_t* pair, bool* value){
   if (!pair || !value){
      return JSON_NULL_ARGUMENT;
   }
   
   if (pair->type != BOOLEAN){
      return JSON_INVALID_ARGUMENT;
   }
   
   *value = pair->value->bVal;
   
   return JSON_SUCCESS;
}

/**
 * This function returns they value type of the pair, This prevents the
 * library user from having to directly access the pair object to 
 * gather any information about it. This will return NIL if the 
 * specified pair is NULL, or if the pair value is actaully NIL. 
 * 
 * @param pair - The JSONKeyValue_t* that you want to find the type of
 * @return - The type of the pair
 */
JSONType_t getPairType(JSONKeyValue_t* pair){
   if (!pair){
      return NIL;
   }
   
   return pair->type;
}

/**
 * This function returns the string value that this pair has. It is 
 * automatically assumed that this pair has a string. If the pair
 * is not a string, it will return NULL. It can also return NULL
 * if the string itself is NULL. If you need more error handling 
 * use the getString() function provided
 * 
 * @param pair - The JSONKeyValue_t* that you want to get the string value from
 * @return - The string value if there is one, NULL otherwise
 * @see getString
 */
const char* getStringVal(JSONKeyValue_t* pair){
   if (!pair){
      return NULL;
   }
   
   if (pair->type == STRING){
      return (const char*)pair->value->sVal;
   }
   
   return NULL;
}


/**
 * This function returns the number value that the pair has. It is 
 * automatically assumed that this pair has a number. If the pair is 
 * not a number a 0.0 is returned. If the actaul value is 0.0 then 0.0 
 * is returned. If you need more error handling use the getNumber()
 * function provided
 * 
 * @param pair - The JSONKeyValue_t* that you want to get the number value from 
 * @return - The number value, or 0.0 if this is not a number.
 * @see getNumber
 */
double getNumberVal(JSONKeyValue_t* pair){
   if (!pair){
      return 0.0;
   }
   
   if (pair->type == NUMBER){
      return pair->value->nVal;
   }
   
   return 0.0;
}


/**
 * This function returns the boolean value that a pair has. It is 
 * automatically assumed that this pair has a boolean value. If the pair
 * is not a boolean, then false is returned. If the actual value of 
 * the boolean is a false, then a false is returned. If you need more
 * error handling use the getBoolean() function provided.
 * 
 * @param pair - The JSONKeyValue_t* that you want to get the boolean value from
 * @return - The boolean value, or false if this is not a boolean
 * @see getBoolean
 */
bool getBooleanVal(JSONKeyValue_t* pair){
   if (!pair){
      return false;
   }
   
   if (pair->type == BOOLEAN){
      return pair->value->bVal;
   }
   
   return false;
}


/**
 * Retreves a list of keys for the elements contained under the passed
 * in element. The order of they keys will be preserved, and nulls will
 * be inserted where JSON nulls exist. 
 * 
 * @param element - the object whos contained keys you are interested in.
 * 
 * @param size - The number of keys found in the search
 * 
 * @return - An array of char* that hold the keys
 * 
 * NOTE: This returns a dynamicly allocated array, remember to free the array,
 * but not the elements when you are done. 
 */
char** getElementKeys(JSONKeyValue_t* element, int* size){
   if (!element || !size){
      *size = -1;
      return NULL;
   }
   
   if (element->type != OBJECT){
      *size = -1;
      return NULL;
   }
   
   //Loop through all of the object elements and add their keys to the array
   char** keys = (char**)malloc(sizeof(char*) * element->length);
   int index = 0;
   JSONKeyValue_t* current = element->value->oVal;
   while(current != NULL){
      keys[index++] = current->key;
      current = current->next;
   }
   
   *size = element->length;
   return keys;
}

/**
 * This disposes of a single pair. This is used by the disposeOfDocument 
 * function, but is also provided as a helper. Please note that if you
 * dispose of a pair that is still attached to a document, it will cause
 * undefined behaiver, and maybe seg faults. 
 * 
 * @param pair - the pair that was previously created
 */
void disposeOfPair(JSONKeyValue_t* pair){
   if (!pair){
      return;
   }
   
   if (pair->type == OBJECT){
      //Objects elements must be freed so we don't get unreachable memory leaks
      JSONKeyValue_t* current = pair->value->oVal;
      JSONKeyValue_t* next;
      while(current != NULL){
         next = current->next;
         disposeOfPair(current);
         current = next;
      }
   }
   else if (pair->type == ARRAY){
      //Arrays elements must be freed so we don't get unreachable memory leaks
      JSONKeyValue_t* current = pair->value->aVal;
      JSONKeyValue_t* next;
      while(current != NULL){
         next = current->next;
         disposeOfPair(current);
         current = next;
      }
   }
   else if (pair->type == STRING){
      //free the string value
      free(pair->value->sVal);
   }
   
   if (pair->key){
      //free the key
      free(pair->key);
   }
   
   //Free the actual value
   free(pair->value);
   free(pair);
}

/**
 * This converts a string with escaped sequences into an unescaped UTF-8 
 * string with all values expanded. All control characters and unicode 
 * escaped sequences will be expanded. Note that the string being created
 * and assigned to converted is a dynamicly created string that must be
 * freed to avoid memory leaks.
 * 
 * @param origional - The origional string that contains JSON escaped sequences.
 * 
 * @param convertedString - This is where the new expanded string will end up. 
 * 
 * @return SUCCESS if the string is converted correctly, error otherwise.
 */
JSONError_t convertString(const char* origional, char** convertedString){
   if (!origional || !convertedString){
      return JSON_NULL_ARGUMENT;
   }
   
   int stringLength = strlen(origional);
   char* converted = (char*)malloc((sizeof(char) * stringLength) + 1);
   
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
   int index = 0;
   char temp[6];
   for (int i = 0; i < stringLength; i++){
      if (origional[i] == '\\'){
         //Found beginning of escae
         i++;
         if (origional[i] == '\\'){
            converted[index++] = '\\';
         }
         else if (origional[i] =='"'){
            converted[index++] = '"';
         }
         else if (origional[i] == '/'){
            converted[index++] = '/';
         }
         else if (origional[i] == 'b'){
            converted[index++] = '\b';
         }
         else if (origional[i] == 'f'){
            converted[index++] = '\f';
         }
         else if (origional[i] == 'n'){
            converted[index++] = '\n';
         }
         else if (origional[i] == 'r'){
            converted[index++] = '\r';
         }
         else if (origional[i] == 't'){
            converted[index++] = '\t';
         }
         else if (origional[i] == 'u'){
            //Need to convert unicode escape sequence to UTF-8
            i++;
            temp[0] = '0';
            temp[1] = 'x';
            for (int j = 0; j < 4; j++){
               if (isxdigit(origional[i])){
                  temp[2+j] = origional[i];
                  i++;
               }
               else{
                  free(converted);
                  return JSON_INVALID_UNICODE_SEQ;
               }
            }
            
            i--; //Need to step back one so we don't miss next character
            
            unsigned int unicode = (unsigned int)strtol(temp, NULL, 16);
            int bytes = convertToUTF8(unicode, temp);
            
            for (int j = 0; j < bytes; j++){
               converted[index++] = temp[j];
            }
         }
         else {
            free(converted);
            return JSON_UNEXPECTED_CHARACTER;
         }
      }
      else {
         //Normal character, just push it into new string
         converted[index++] = origional[i];
      }
   }
   
   converted[index] = '\0';
   //converted = (char*)realloc(converted, index + 1);
   
   *convertedString = converted;
   return JSON_SUCCESS;
}


static int convertToUTF8(unsigned int character, char utfBytes[]){
   if (!utfBytes){
      return -1;
   }
   
   //ASCII characters are just left alone
   if (character <= 0x7F){
      utfBytes[0] = (char)(character & 0xFF);
      return 1;
   }
   else if (character > 0x7F && character <= 0x7FF){
      /*
       * 00000yyy yyxxxxxx origional 
       * 110yyyyy 10xxxxxx formatted
       */
      char x = (character & 0x3F);
      char y = ((character & 0x07C0) >> 6);
      y |= 0xC0;
      x |= 0x80;
      
      utfBytes[0] = y;
      utfBytes[1] = x;
      return 2;
   }
   else if (character > 0x7FF && character <= 0xFFFF){
      /*
       * zzzzyyyy yyxxxxxx          origional
       * 1110zzzz 10yyyyyy 10xxxxxx formatted
       */
       
       char z = ((character & 0xF000) >> 12);
       char y = ((character & 0x0FC0) >> 6);
       char x = (character & 0x3F);
       
       z |= 0xE0;
       y |= 0x80;
       x |= 0x80;
       
       utfBytes[0] = z;
       utfBytes[1] = y;
       utfBytes[2] = x;
       return 3;
   }
   else {
      return -1;
   }
}
