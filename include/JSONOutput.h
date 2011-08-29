#ifndef _JSON_OUTPUT_H
#define _JSON_OUTPUT_H

#include "JSONCommon.h"

#ifdef CPP
extern "C" {
#endif

JSONError_t documentToString(JSONKeyValue_t* document, char** output, int* length);

#ifdef CPP
}
#endif

#endif
