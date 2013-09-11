#ifndef _JSON_OUTPUT_H
#define _JSON_OUTPUT_H

#include "jsoncommon.h"

#ifdef __cplusplus
extern "C" {
#endif

JSONError_t documentToString(JSONKeyValue_t* document, char** output, int* length);

#ifdef __cplusplus
}
#endif

#endif
