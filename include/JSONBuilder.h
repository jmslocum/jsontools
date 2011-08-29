#ifndef _JSON_BUILDER_H
#define _JSON_BUILDER_H

#include <stdio.h>
#include <stdbool.h>
#include "JSONCommon.h"

#ifdef CPP
extern "C" {
#endif

JSONValue_t* newJSONString(char* string);
JSONValue_t* newJSONNumber(double number);
JSONValue_t* newJSONBoolean(bool boolean);
JSONValue_t* newJSONObject(JSONKeyValue_t* pair);
JSONValue_t* addKeyValuePair(JSONValue_t* object, JSONKeyValue_t* pair);
JSONKeyValue_t* newJSONArray(char* key, void* array[], JSONType_t types[], int length);
JSONKeyValue_t* newJSONPair(JSONType_t type, char* key, JSONValue_t* value);

#ifdef CPP
}
#endif

#endif
