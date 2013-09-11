#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <getopt.h>
#include <unistd.h>
#include <syslog.h>

#include <config.h>
#include "jsontools.h"

static bool version = false;
static bool help = false;
static bool verify = false;
static bool standardin = false;
static char* key = NULL;
static char* delimit = NULL;

static struct option longOptions[] = {
  {"help",    no_argument,       NULL,   'h'},
  {"version", no_argument,       NULL,   'v'},
  {"verify",  no_argument,       NULL,   'r'},
  {"key",     required_argument, NULL,   'k'},
  {"delimit", required_argument, NULL,   'd'},
  { 0,        0,                 0,       0 }
};

static const char* shortOptions = "hvrk:d:";
static char* findValueForKey(char* key, char* delimit, JSONKeyValue_t* document);

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
  fprintf(term, "\t%s [options] [files]\n", programName);
  fprintf(term, "options:\n");
  fprintf(term, "\t-h  --help    Print this messsage\n");
  fprintf(term, "\t-v  --version Print the version number\n");
  fprintf(term, "\t-r  --verify  only a 0 or 1 return value\n");
  fprintf(term, "\t              0 = good json, positive number = bad\n");
  fprintf(term, "\t-k  --key     Print the value of the given key\n");
  fprintf(term, "\t              only for string, number, bool, or null\n");
  fprintf(term, "\t-d  --delimit Specify a seperator for multi-level keys\n");
  exit(exitCode);
}

/**
  This function is a custom memory reallocation function that will expand
  a memory space pointed to by the pointer. This newly expanded space will 
  be filled with zeros (if the new space is larger then the old space)

  @param ptr - The origional pointer, or NULL for a new pointer
  @param oldSize - The origional size of the memory allocated
  @param newSize - The new size of the memory after allocation
  @return The new pointer
*/
static void* crealloc(void* ptr, int oldSize, int newSize){
  void* newPtr = NULL;
  if (oldSize >= newSize){
    newPtr = realloc(ptr, newSize);
    if (!newPtr){
      return NULL;
    }
  }
  else {
    newPtr = malloc(newSize);
    if (!newPtr) return NULL;

    memset(newPtr, 0, newSize);
    if (ptr){
      memcpy(newPtr, ptr, oldSize);
    }
  }

  return newPtr;
}

/**
  Read the message from standard in and return the message back to the
  the caller as a char*
  @return The json message from the console
*/
static char* readFromConsole(){
  int charCount = 100;
  int charIndex = 0;
  char read = '\0';
  char* input = (char*)crealloc(NULL, 0, charCount);
  if (!input){
    return NULL;
  }

  while((read = getchar()) != EOF){
    input[charIndex++] = read;
    if (charIndex >= charCount){
      input = (char*)crealloc(input, (sizeof(char) * charCount), (sizeof(char) * charCount * 2));
      charCount *= 2;
      if (!input){
        return NULL;
      }
    }
  }

  return input;
}

/**
  Read the message from a file, and return the message back to the 
  caller as a char*
  @param fileName - The name of the file to read the message from
  @return The json message from the file
*/
static char* readFromFile(const char* fileName, long long *dataRead){
  FILE* jsonMessage = NULL;
  jsonMessage = fopen(fileName, "r");
  if (!jsonMessage){
    return NULL;
  }

  fseek(jsonMessage, 0, SEEK_END);
  long long fileSize = ftell(jsonMessage);
  fseek(jsonMessage, 0, SEEK_SET);

  char* message = (char*)crealloc(NULL, 0, (sizeof(char) * (fileSize + 1)));
  if (!message){
    return NULL;
  }

  *dataRead = fread(message, sizeof(char), fileSize, jsonMessage);
  fclose(jsonMessage);

  return message;
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
      case 'k' :
         key = strdup(optarg);
         break;
      case 'd' :
         delimit = strdup(optarg);
         break;
      case '?' :
        break;
      default :
        usageAndExit(stderr, argv[0], 1);
    }
  }

  return optIndex;
}

static char* findValueForKey(char* key, char* delimit, JSONKeyValue_t* document){
   
   return NULL;
}


/**
  The main function for the program
*/
int main(int argc, char* argv[]) {
  int exitcode = EXIT_SUCCESS;
  char* message = NULL;

  //Set up the syslog 
  openlog(argv[0], LOG_PID, LOG_USER);


  if (argc < 1) {
    standardin = true;
  }

  parseCommandlineOptions(argc, argv);

  if (key){
     if (!delimit){
        delimit = strdup(".");
     }
  }

  if (help){
    usageAndExit(stdout, argv[0], 0);
  }

  if (version) {
    printf("author: James Slocum\n");
    printf("version: %s\n", VERSION); //from config.h
    exit(0);
  }
  
  int fileCount = 0;

  if (standardin){
    fileCount = 1;
  }
  else if (optind < argc) {
    fileCount = argc - optind;
  }
  
  else {
    standardin = true;
    fileCount = 1;
  }

  while (fileCount > 0){
    fileCount--;
    long long dataRead = 0;
    const char* filename = "";
    if (standardin){
      message = readFromConsole();
      filename = "stdin";
    }
    else {
      message = readFromFile(argv[optind++], &dataRead);
      filename = argv[optind - 1];
    }

    if (!message){
      fprintf(stderr, "Unable to read from file %s", filename);
      exit(1);
    }

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
          fprintf(stderr, "Error while parsing line %d\n", parser->lineNumber);
          fprintf(stderr, "%s\n", errorReport);
        }
       
        disposeOfPair(document);
        free(message);
        free(parser);
        exit(status);
      }

      if (key){
         char* value = findValueForKey(key, delimit, document);
         if (!value){
            fprintf(stdout, "key not found!\n");
            exitcode = 1;
         }
      }
      else {
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
      }

      disposeOfPair(document);

    } while (status == JSON_SUCCESS && lastIndex > 0);

    free(message);
    free(parser);

  }

  //Close out the system log
  closelog();
  return exitcode;
}

