#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include "version.h"
#include "JSONTools.h"

static bool version = false;
static bool help = false;
static bool verify = false;

static struct option longOptions[] = {
  {"help",    no_argument,    NULL,   'h'},
  {"version", no_argument,    NULL,   'v'},
  {"verify",  no_argument,    NULL,   'r'},
  { 0,        0,              0,       0 }
};

static const char* shortOptions = "hvr";


/**
  This function will print the help menu and exit with the
  specified exit code
  @param term - The location to write the output, if NULL then output is sent to stdout
  @param programName - The name of this program (as the user typed it)
  @param exitCode - The exit code to report back to the system
*/
static void usageAndExit(FILE* term, char* programName, int exitCode){
  if (!term) term = stdout;
  fprintf(term, "%s usage: \n", programName);
  fprintf(term, "\t%s [options] file1 file2 ... fileN\n", programName);
  fprintf(term, "options:\n");
  fprintf(term, "\t-h  --help    Print this messsage\n");
  fprintf(term, "\t-v  --version Print the version number\n");
  fprintf(term, "\t-r  --verify  only a 0 or 1 return value\n");
  fprintf(term, "\t              0 = good json, positive number = bad\n");
  exit(exitCode);
}

/**
  This function will parse through the command line options and
  set the proper variables. 

  @param argc - The number of arguments
  @param argv - The command line arguments
*/
static int parseCommandlineOptions(int argc, char* argv[]){
  char c;
  int optIndex = 0;
  while ((c = getopt_long(argc, argv, shortOptions, longOptions, &optIndex)) != -1){
    switch (c){
      case 'h' :
        help = true;
        break;
      case 'v' :
        version = true;
        break;
      case 'r' :
        verify = true;
        break;
      case '?' :
        break;
      default :
        usageAndExit(stderr, argv[0], 1);
    }
  }

  return optIndex;
}

/**
  The main function for the program
*/
int main(int argc, char* argv[]){
  if (argc <= 1) {
    usageAndExit(stderr, argv[0], 1);
  }

  int lastOptionIndex = parseCommandlineOptions(argc, argv);

  if (help){
    usageAndExit(stdout, argv[0], 0);
  }

  if (version) {
    printf("author: James Slocum\n");
    printf("version: %s\n", VERSION);
    printf("revision: \"%s\"\n", REVISION);
    exit(0);
  }
  
  int fileCount = 0;
  FILE* jsonMessage = NULL;

  if (lastOptionIndex < argc) {
    fileCount = argc - optind;
  }

  while (fileCount > 0){
    fileCount--;
    jsonMessage = fopen(argv[optind], "r");
    if (!jsonMessage){
      fprintf(stderr, "Unable to open file %s", argv[optind - 1]);
      exit(1);
    }

    fseek(jsonMessage, 0, SEEK_END);
    long long fileSize = ftell(jsonMessage);
    fseek(jsonMessage, 0, SEEK_SET);

    char* message = malloc((fileSize + 1) * sizeof(char));
    if (!message){
      fclose(jsonMessage);
      fprintf(stderr, "Unable to allocate memory!");
      exit(1);
    }

    long long dataRead = fread(message, sizeof(char), fileSize, jsonMessage);
    fclose(jsonMessage);
    if (dataRead != fileSize){
      fprintf(stderr, "only read %lld of %lld bytes", dataRead, fileSize);
      exit(1);
    }
  
    //Add the terminator character before we pass the message to the parser
    message[fileSize] = '\0'; 

    JSONParser_t* parser = newJSONParser();
    JSONKeyValue_t* document = NULL;
    int lastIndex = 0;
    int currentIndex = 0;
    JSONError_t status = 0;

    do {
      status = parseJSONMessage(parser, &document, &message[currentIndex], &lastIndex);
      currentIndex += lastIndex;

      if (status){
        //An error has occurred! if we are not in verify mode, print the error
        //otherwise, just exit with status 1
        if (!verify) {
          const char* errorReport = json_strerror(json_errno);
          fprintf(stderr, "%s\n", errorReport);
        }
       
        disposeOfPair(document);
        free(message);
        free(parser);
        exit(status);
      }

      char* parsedDocument = NULL;
      int messageLength;
      status = documentToString(document, &parsedDocument, &messageLength);

      if (status){
        if (!verify){
          const char* errorReport = json_strerror(json_errno);
          fprintf(stderr, "%s\n", errorReport);
        }
      }

      if (!verify){
        fprintf(stdout, "%s\n", parsedDocument);
      }

      if (parsedDocument){
        free(parsedDocument);
      }
      disposeOfPair(document);

    } while (status == JSON_SUCCESS && lastIndex > 0);

    free(message);
    free(parser);

  }

  return EXIT_SUCCESS;
}

