#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "jsonerror.h"
#include "jsonparser.h"

int json_errno;

static char* errorDescriptions[] = {
   "Success",
   "Unknown Failure",
   "The parser is in a bad state, and cannot be used to parse this message",
   "The argument passed to a library function was NULL",
   "The value in a key:value pair was NULL when it should not have been",
   "The key in a key:value pair was NULL when it should not have been",
   "The message is not valid JSON (no '{' found?)",
   "The argument passed to a library function is of the wrong type",
   "The type specified in a pair does not exist",
   "The unicode sequence was not valid",
   "The value being parsed is not a valid for any type",
   "The key being parsed is not a valid json key",
   "The string contains characters that are not allowed by JSON",
   "An unexpected closing bracket was found while parsing the message",
   "An unexpected closing brace was found while parsing the message",
   "A numeric value was found where there should not be one",
   "A string was found (quote really) where there should not be one",
   "A value was found instead of a key",
   "A null was found where it wasn't expected (perhapse after a key)",
   "A boolean value was found where it wasnt expected",
   "A key was found instead of a value or delimiter",
   "An object '{' was found where there should not be one",
   "An unquoted character was found that was not 't' 'f' or 'n'",
   "An array '[' was found where there should not be one",
   "a delimiter ':' was found where there should not be one",
   "a comma ',' was found where there should not be one",
   "The message being parsed has over 256 nested objects",
   "The eof has hit before the closing '}' was found",
   "The number value is out of range for a double type",
   "The pair that was being searched for was not found",
   "Unable to allocate memory for json object",
   "A stdlib function failed",
};


/**
 * Returns a description of the specific error number
 */
const char* json_strerror(int errNo){
   if (errNo < 0 || errNo > JSON_INTERNAL_FAILURE){
      return "UNKNWON ERROR";
   }
   
   return errorDescriptions[errNo];
}
