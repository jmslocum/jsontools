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
   
   long long sizeForTest = fileSize;
   fread(message, sizeof(char), sizeForTest, jsonMessage);
   fclose(jsonMessage);
   
   message[sizeForTest] = '\0';
   
   JSONParser_t* parser = newJSONParser();
   JSONKeyValue_t* document;
   int lastIndex = 0;
   int currentIndex = 0;
   JSONError_t status;
   
   do {
      status = parseJSONMessage(parser, &document, &message[currentIndex], &lastIndex);
      currentIndex += lastIndex;
      
      if (status){
         char* errorReport = parser->tracebackString;
         fprintf(stderr, "%s\n", errorReport);
         
         for (int i = parser->keyStackIndex - 1; i >= 0; i--){
            fprintf(stderr, "key->%s\n", parser->keyStack[i]);
         }
         
         exit(status);
      }
      
      char* parsedDocument;
      int messageLength;
      status = documentToString(document, &parsedDocument, &messageLength);
      
      if (status){
         const char* errorReport = json_strerror(json_errno);
         fprintf(stderr, "%s\n", errorReport);
         exit(status);
      }
      
      fprintf(stdout, "%s\n", parsedDocument);
      free(parsedDocument);
      disposeOfPair(document);
      
   } while(status == JSON_SUCCESS && lastIndex > 0);
   
   free(parser);
   return 0;
}
