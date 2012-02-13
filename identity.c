#include "ta/identity.h"

/*  taListenPhraseCountMax == 16  (in the javascript)                        */
#define MAX_PHRASE_LAG 12

#if defined TA_IDENTITY_IMPL_LONG_LIST
#include "ta/identity.impl.long_list.c"
#elif defined TA_IDENTITY_IMPL_QUEUE
#include "ta/identity.impl.queue.c"
#elif defined TA_IDENTITY_IMPL_ZAP_LIST
#include "ta/identity.impl.zap_list.c"
#else
#include "ta/identity.impl.zap_list.c"
#endif
