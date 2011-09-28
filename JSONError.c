#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "JSONError.h"

int json_errno;

static char* errorDescriptions[] = {
   "Success",
   "Unknown Failure",
   "The argument passed to a library function was NULL",
   "The value in a key:value pair was NULL when it should not have been",
   "The key in a key:value pair was NULL when it should not have been",
   "The message is not valid JSON",
   "The argument passed to a library function is of the wrong type",
   "The type specified in a pair does not exist",
   "The string contains characters that are not allowed by JSON",
   "A recurrsive call faied, calling the current call to fail",
   "An extra closing bracket was found while parsing the message",
   "An extra closing brace was found while parsing the message",
   "The end of the message was reached in the middle of parsing",
   "A numeric value was found where there should not be one",
   "A string was found (quote really) where there should not be one",
   "A value was found instead of a key",
   "A key was found instead of a value or delimiter",
   "An object '{' was found where there should not be one",
   "An unquoted character was found where there should not be one",
   "An array '[' was found where there should not be one",
   "a delimiter ':' was found where there should not be one",
   "The message being parsed has over 256 nested objects",
   "The message ends unexpectdly",
   "The number value is out of range for a double type",
   "The pair that was being searched for was not found",
   "A stdlib function failed",
};

/**
 * The globally shared error stack
 */
JSONErrorStack_t json_errorStack;

/**
 * Push an error onto the stack to be reported back to the calling function
 * NOTE that this function should be accessed via the PUSH_ERROR macro
 * 
 * @param error - The error that occurred
 * @param currentFunction - The name of the function that is currently executing
 * @param currentFile - The name of the source file that is being executed
 * @param description - A breif, readable description of the problem
 */
void pushError(JSONError_t error, const char* currentFunction, const char* currentFile, const char* description){
   if (json_errorStack.stackIndex < KEY_STACK_SIZE){
      //Need to make copies of the strings to prevent segfaults
      char* functionCopy = (char*)malloc(sizeof(char) * (strlen(currentFunction)));
      char* fileCopy = (char*)malloc(sizeof(char) * (strlen(currentFile)));
      char* descriptionCopy = (char*)malloc(sizeof(char) * (strlen(description)));
      
      strcpy(functionCopy, currentFunction);
      strcpy(fileCopy, currentFile);
      strcpy(descriptionCopy, description);
      
      json_errorStack.errors[json_errorStack.stackIndex] = error;
      json_errorStack.functions[json_errorStack.stackIndex] = functionCopy;
      json_errorStack.files[json_errorStack.stackIndex] = fileCopy;
      json_errorStack.descriptions[json_errorStack.stackIndex] = descriptionCopy;
      
      json_errorStack.stackIndex++;
   }
}

/**
 * Returns the error report in a printable format that can be presented to
 * the user.
 * 
 * @return A string with the stack trace and the errors that occurred.
 */
char* getErrorReport(){
   int outputSize = 128;
   char* output = (char*)malloc(sizeof(char) * (outputSize + 1));
   int outputIndex = 0;
   char temp[256];
   
   if (!output){
      return NULL;
   }
   
   int errorLength = 0;
   for (int i = json_errorStack.stackIndex - 1; i >= 0; i--) {
      errorLength = sprintf(temp, "%s:%s() -> %s\n", json_errorStack.files[i], 
                                                     json_errorStack.functions[i], 
                                                     json_errorStack.descriptions[i]);
                                                     
      if (outputIndex + errorLength > outputSize){
         outputSize *= 2;
         output = (char*) realloc(output, (sizeof(char) * (outputSize + 1)));
         if (output == NULL){
            return NULL;
         }
      }
      
      strcat(output, temp);
   }
   
   return output;
}

/**
 * Clears all of the errors out of the stack, and gets it ready for a new
 * run.
 */
void clearErrorStack(){
   for (int i = json_errorStack.stackIndex - 1; i >= 0; i--){
      free(json_errorStack.functions[i]);
      free(json_errorStack.files[i]);
      free(json_errorStack.descriptions[i]);
   }
   
   json_errorStack.stackIndex = 0;
}

/**
 * Returns a description of the specific error number
 */
const char* json_strerror(int errno){
   if (errno < 0 || errno > INTERNAL_FAILURE){
      return errorDescriptions[1];
   }
   
   return errorDescriptions[errno];
}
