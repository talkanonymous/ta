#ifndef ta_identity_h
#define ta_identity_h

#include "ta/phrase.h"
#include "xt/case/list.h"
#include "xt/core/bool.h"
#include "xt/core/log.h"

#define TA_DEFAULT_IDENTITY_NAME "anonymous"

struct ta_identity_t;
typedef struct ta_identity_t ta_identity_t;

void ta_identity_clear(ta_identity_t *identity);

int ta_identity_compare(void *identity_object_a, void *identity_object_b);

xt_core_bool_t ta_identity_compare_equal(void *identity_object_a,
    void *identity_object_b);

void *ta_identity_copy(void *identity_object);

ta_identity_t *ta_identity_create(char *name, xt_core_log_t *log);

ta_identity_t *ta_identity_create_decoy(char *name, xt_core_log_t *log);

void ta_identity_destroy(void *identity_object);

void ta_identity_destroy_decoy(void *identity_decoy_object);

unsigned long ta_identity_get_size(ta_identity_t *identity);

unsigned long ta_identity_hash(void *identity_object);

xt_case_list_t *ta_identity_listen(ta_identity_t *identity,
    unsigned long next_phrase_id);

unsigned long ta_identity_mod(void *identity_object, unsigned long divisor);

xt_core_bool_t ta_identity_talk(ta_identity_t *identity, ta_phrase_t *phrase);

#endif
