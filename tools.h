#ifndef ta_tools_h
#define ta_tools_h

#include "xt/core/standard.h"

#define TA_ENGINE_ID_MIN 32
#define TA_ENGINE_ID_MAX 32
#define TA_ENGINE_TYPE_COUNT 1
enum ta_engine_id_t {
  TA_ENGINE_HTTP = 32,
};
typedef enum ta_engine_id_t ta_engine_id_t;

void ta_decode_url(char *pszDecodedOut, size_t nBufferSize,
    const char *pszEncodedIn);

char *ta_get_engine_name(unsigned long engine_id);

#endif
