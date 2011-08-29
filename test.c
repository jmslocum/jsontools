#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "JSONTools.h"

/*------------------------------------------------------------------
 * Main function for testing the parser
 *-----------------------------------------------------------------*/
int main(int argc, char **argv)
{
   if (argc < 2){
      fprintf(stdout, "Please specify a json fle name\n");
      exit(1);
   }
   
   FILE* jsonMessage = fopen(argv[1], "r");
   if (!jsonMessage){
      fprintf(stderr, "Unable to open file %s\n", argv[1]);
      exit(2);
   }
   
   fseek(jsonMessage, 0, SEEK_END);
   long long fileSize = ftell(jsonMessage);
   fseek(jsonMessage, 0, SEEK_SET);
   
   char message[fileSize];
   
   fread(message, sizeof(char), fileSize, jsonMessage);
   fclose(jsonMessage);
   
   JSONParser_t* parser = newJSONParser();
   JSONKeyValue_t* document;
   int lastIndex = 0;
   JSONError_t status = parseJSONMessage(parser, &document, message, &lastIndex);
   
   if (status){
      char* errorReport = getErrorReport();
      fprintf(stderr, "%s\n", errorReport);
      
      for (int i = parser->keyStackIndex - 1; i >= 0; i--){
         fprintf(stderr, "key->%s\n", parser->keyStack[i]);
      }
      
      fprintf(stderr, "line = %d\n", parser->lineNumber);
      fprintf(stderr, "index = %d\n", parser->index);
      exit(status);
   }
   
   //Test the remove function
   if (!removeChildPair(document, "unicodeString")){
      fprintf(stdout, "Found and removed unicodeString element\n");
   }
   
   if (!removeChildPair(document, "array")){
      fprintf(stdout, "Found and removed array element\n");
   }
   
   char* parsedDocument;
   int messageLength;
   status = documentToString(document, &parsedDocument, &messageLength);
   
   if (status){
      char* errorReport = getErrorReport();
      fprintf(stderr, "%s\n", errorReport);
      exit(status);
   }
   
   fprintf(stdout, "%s\n", parsedDocument);
   free(parsedDocument);
   
   int size = 0;
   char** documentKeys = getElementKeys(document, &size);
   JSONKeyValue_t* temp = NULL;
   for (int i = 0; i < size; i++){
      fprintf(stdout, "key->%s\n", documentKeys[i]);
      if ((getChildPair(document, documentKeys[i]))->type == OBJECT){
         temp = getChildPair(document, documentKeys[i]);
         fprintf(stdout, "\tThis is an object\n");
         int tempSize = 0;
         char** tempKeys = getElementKeys(temp, &tempSize);
         for (int j = 0; j < tempSize; j++){
            fprintf(stdout, "\tobjKey->%s\n", tempKeys[j]);
         }
         free(tempKeys);
      }
   }
   
   JSONKeyValue_t* unicode = getChildPair(document, "unicodeString");
   char* conv;
   if (unicode){
      if (convertString(unicode->value->sVal, &conv) == SUCCESS){
         fprintf(stdout, "%s\n", conv);
      }
      else{
         fprintf(stdout, "Unable to convert unicode string\n");
      }
   }
   
   //free(conv);
   free(documentKeys);
   disposeOfPair(document);
   free(parser);
	
	return 0;
}
