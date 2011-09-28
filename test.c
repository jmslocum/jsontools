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
   //fclose(jsonMessage);
   
   message[sizeForTest] = '\0';
   
   JSONParser_t* parser = newJSONParser();
   JSONKeyValue_t* document;
   int lastIndex = 0;
   JSONError_t status = parseJSONMessage(parser, &document, message, &lastIndex);
   
   while(status == MESSAGE_INCOMPLETE){
      //fread(message, sizeof(char), sizeForTest, jsonMessage);
      fprintf(stdout, "Retry parsing message...\n");
      message[sizeForTest++] = fgetc(jsonMessage);
      message[sizeForTest] = '\0';
      resetParser(parser);
      status = parseJSONMessage(parser, &document, message, &lastIndex);
   }
   
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
   
   //free(conv);
   disposeOfPair(document);
   free(parser);
	
	return 0;
}
