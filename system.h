#ifndef ta_system_h
#define ta_system_h

#include "ta/phrase.h"
#include "xt/case/list.h"
#include "xt/core/log.h"

struct ta_system_t;
typedef struct ta_system_t ta_system_t;

ta_system_t *ta_system_create(unsigned long identities_shard_count,
    unsigned short clear_all_identities_period_minutes, xt_core_log_t *log);

void ta_system_destroy(ta_system_t *system);

xt_case_list_t *ta_system_listen(ta_system_t *system, char *identity_name,
    unsigned long next_phrase_id);

void ta_system_maintain(ta_system_t *system);

xt_core_bool_t ta_system_talk(ta_system_t *system, char *identity_name,
    ta_phrase_t *phrase);

#endif
