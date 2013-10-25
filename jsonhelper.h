#ifndef JSON_HELPER_H
#define JSON_HELPER_H
 
#include <stdio.h>
#include <stdbool.h>
#include "jsoncommon.h"
#include "jsonerror.h"

#ifdef __cplusplus
extern "C" {
#endif

bool hasChildPair(JSONKeyValue_t* parent, const char* key);
JSONKeyValue_t* getChildPair(JSONKeyValue_t* parent, const char* key);
JSONKeyValue_t* getAllChildPairs(JSONKeyValue_t* parent);
JSONError_t getArray(JSONKeyValue_t* pair, JSONKeyValue_t** values); 
JSONError_t getString(JSONKeyValue_t* pair, char** value);
JSONError_t getNumber(JSONKeyValue_t* pair, double* value);
JSONError_t getBoolean(JSONKeyValue_t* pair, bool* value);
JSONType_t getPairType(JSONKeyValue_t* pair);
const char* getStringVal(JSONKeyValue_t* pair);
double getNumberVal(JSONKeyValue_t* pair);
bool getBooleanVal(JSONKeyValue_t* pair);
char** getElementKeys(JSONKeyValue_t* element, int* size);
void disposeOfPair(JSONKeyValue_t* pair);
JSONError_t convertString(const char* origional, char** coverted);

#ifdef __cplusplus
}
#endif

#endif
