#include "ta/tools.h"
#include "xt/core/bool.h"

static char *engine_names[TA_ENGINE_TYPE_COUNT] = {
  "http",
};

void ta_decode_url(char *pszDecodedOut, size_t nBufferSize,
    const char *pszEncodedIn)
{
  unsigned int i;
  unsigned int j;
  xt_core_bool_t bBothDigits;
  memset(pszDecodedOut, 0, nBufferSize);

  enum DecodeState_e {
    STATE_SEARCH = 0,  /* searching for an ampersand to convert */
    STATE_CONVERTING,  /* convert the two proceeding characters from hex */
  };

  enum DecodeState_e state = STATE_SEARCH;

  for (i = 0; i < strlen(pszEncodedIn) - 1; ++i) {
    switch (state) {
      case STATE_SEARCH:
        if (pszEncodedIn[i] != '%') {
          strncat(pszDecodedOut, &pszEncodedIn[i], 1);
          break;
        }
        /* We are now converting */
        state = STATE_CONVERTING;
        break;

      case STATE_CONVERTING:
        /* Conversion complete (i.e. don't convert again next iter) */
        state = STATE_SEARCH;

        /*
          Create a buffer to hold the hex. For example, if %20, this buffer
          would hold 20 (in ASCII)
        */
        char pszTempNumBuf[3] = {0};
        strncpy(pszTempNumBuf, &pszEncodedIn[i], 2);

        /* Ensure both characters are hexadecimal */
        bBothDigits = xt_core_bool_true;

        for (j = 0; j < 2; ++j) {
          if(!isxdigit(pszTempNumBuf[j])) {
            bBothDigits = xt_core_bool_false;
          }
        }

        if (!bBothDigits) {
          break;
        }

        /* Convert two hexadecimal characters into one character */
        int nAsciiCharacter;
        sscanf(pszTempNumBuf, "%x", &nAsciiCharacter);

        /* Ensure we aren't going to overflow */
        assert(strlen(pszDecodedOut) < nBufferSize);

        /* Concatenate this character onto the output */
        strncat(pszDecodedOut, (char*)&nAsciiCharacter, 1);

        /* Skip the next character */
        i++;
        break;
    }
  }
}

char *ta_get_engine_name(unsigned long engine_id)
{
  char *engine_name;

  if ((engine_id >= TA_ENGINE_ID_MIN) && (engine_id <= TA_ENGINE_ID_MAX)) {
    engine_name = engine_names[engine_id - TA_ENGINE_ID_MIN];
  } else {
    engine_name = "[unknown]";
  }

  return engine_name;
}
