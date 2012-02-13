#include "ta/identity.h"
#include "ta/system.h"
#include "xt/case/shardset.h"
#include "xt/sync/period.h"

#define TA_SYSTEM_IDENTITY_NAME "::ta"
#define TA_SYSTEM_SHUTDOWN_TEXT "talk [?] anonymous is shutting down for " \
  "maintenance..we'll be back asap"

struct ta_system_t {
  xt_case_shardset_t *identities;
  xt_sync_period_t *clear_all_identities_period;
  unsigned short clear_all_identities_period_minutes;
  xt_core_log_t *log;
};

static void clear_all_identities(ta_system_t *system);

static ta_identity_t *find_or_create_identity(ta_system_t *system,
    char *identity_name);

static xt_core_bool_t identity_is_vacant(void *identity_object);

/*
static xt_core_bool_t talk_all_identities(ta_system_t *system, char *text);
*/

void clear_all_identities(ta_system_t *system)
{
  assert(system);
  ta_identity_t *identity;

  xt_case_shardset_iterate_start(system->identities);
  while ((identity = xt_case_shardset_iterate_next(system->identities))) {
    ta_identity_clear(identity);
  }
}

ta_identity_t *find_or_create_identity(ta_system_t *system,
    char *identity_name)
{
  assert(system);
  assert(identity_name);
  ta_identity_t *identity = NULL;
  ta_identity_t *identity_decoy;

  identity_decoy = ta_identity_create_decoy(identity_name, system->log);
  if (identity_decoy) {
    identity = xt_case_shardset_find(system->identities, identity_decoy);
    ta_identity_destroy_decoy(identity_decoy);
    if (!identity) {
      identity = ta_identity_create(identity_name, system->log);
      if (identity) {
        if (!xt_case_shardset_add(system->identities, identity)) {
          xt_core_log_trace(system->log, " ta ", "xt_case_shardset_add");
          ta_identity_destroy(identity);
        }
      } else {
        xt_core_log_trace(system->log, " ta ", "ta_identity_create");
      }
    }
  } else {
    xt_core_log_trace(system->log, " ta ", "ta_identity_create_decoy");
  }

  return identity;
}

xt_core_bool_t identity_is_vacant(void *identity_object)
{
  assert(identity_object);
  ta_identity_t *identity = identity_object;
  return (ta_identity_get_size(identity) == 0);
}

ta_system_t *ta_system_create(unsigned long identities_shard_count,
    unsigned short clear_all_identities_period_minutes, xt_core_log_t *log)
{
  ta_system_t *system;
  xt_core_bool_t so_far_so_good;

  system = malloc(sizeof *system);
  if (system) {
    system->log = log;
    system->clear_all_identities_period = NULL;
    system->clear_all_identities_period_minutes
      = clear_all_identities_period_minutes;
    system->identities = xt_case_shardset_create(ta_identity_compare,
        ta_identity_compare_equal, ta_identity_copy, ta_identity_destroy,
        ta_identity_hash, ta_identity_mod, identities_shard_count);
    if (system->identities) {
      so_far_so_good = xt_core_bool_true;
    } else {
      so_far_so_good = xt_core_bool_false;
      xt_core_log_trace(log, " ta ", "xt_case_shardset_create");
    }
  } else {
    so_far_so_good = xt_core_bool_false;
    xt_core_log_trace(log, " ta ", "malloc");
  }

  if (so_far_so_good) {
    system->clear_all_identities_period
      = xt_sync_period_create(clear_all_identities_period_minutes * 60);
    if (!system->clear_all_identities_period) {
      xt_core_log_trace(log, " ta ", "xt_sync_period_create");
    }
  }

  if (system && !so_far_so_good) {
    if (system->identities) {
      xt_case_shardset_destroy(system->identities);
    }
    if (system->clear_all_identities_period) {
      xt_sync_period_destroy(system->clear_all_identities_period);
    }
    free(system);
    system = NULL;
  }

  return system;
}

void ta_system_destroy(ta_system_t *system)
{
  assert(system);

  /*
  if (talk_all_identities(system, TA_SYSTEM_SHUTDOWN_TEXT)) {
    sleep(8);
  } else {
    xt_core_log_trace(system->log, " ta ", "talk_all_identities");
  }
  */

  xt_case_shardset_destroy(system->identities);
  xt_sync_period_destroy(system->clear_all_identities_period);
  free(system);
}

xt_case_list_t *ta_system_listen(ta_system_t *system, char *identity_name,
    unsigned long next_phrase_id)
{
  assert(system);
  assert(identity_name);
  ta_identity_t *identity;
  xt_case_list_t *phrases;

  /*
    TODO: should this be find-or-create .. or just find?
  */
  identity = find_or_create_identity(system, identity_name);
  if (identity) {
    phrases = ta_identity_listen(identity, next_phrase_id);
    if (!phrases) {
      xt_core_log_trace(system->log, " ta ", "ta_identity_listen");
    }
  } else {
    phrases = NULL;
    xt_core_log_trace(system->log, " ta ", "find_or_create_identity");
  }

  return phrases;
}

void ta_system_maintain(ta_system_t *system)
{
  assert(system);
  unsigned long removed_identity_count;
  char text[128];
  ta_phrase_t *phrase;

  removed_identity_count = xt_case_shardset_remove_if(system->identities,
      identity_is_vacant);
  if (removed_identity_count > 0) {
    xt_core_log_enter(system->log, " ta ", "removed %lu vacant identities",
        removed_identity_count);
  }

  if (xt_sync_period_once(system->clear_all_identities_period)) {
    xt_core_log_enter(system->log, " ta ", "clear all identities");
    clear_all_identities(system);
  }

  if (snprintf(text, 128, "%lu active identities",
          xt_case_shardset_get_size(system->identities)) >= 0) {
    xt_core_log_enter(system->log, " ta ", text);
    phrase = ta_phrase_create(TA_PHRASE_NO_ID, text, strlen(text));
    if (phrase) {
      if (!ta_system_talk(system, TA_SYSTEM_IDENTITY_NAME, phrase)) {
        xt_core_log_trace(system->log, " ta ", "ta_system_talk");
        ta_phrase_destroy(phrase);
      }
    } else {
      xt_core_log_trace(system->log, " ta ", "ta_phrase_create");
    }
  } else {
    xt_core_log_trace(system->log, " ta ", "snprintf");
  }
}

xt_core_bool_t ta_system_talk(ta_system_t *system, char *identity_name,
    ta_phrase_t *phrase)
{
  assert(system);
  assert(identity_name);
  ta_identity_t *identity;
  xt_core_bool_t success;

  identity = find_or_create_identity(system, identity_name);
  if (identity) {
    if (ta_identity_talk(identity, phrase)) {
      success = xt_core_bool_true;
    } else {
      success = xt_core_bool_false;
      xt_core_log_trace(system->log, " ta ", "ta_identity_talk");
    }
  } else {
    success = xt_core_bool_false;
    xt_core_log_trace(system->log, " ta ", "find_or_create_identity");
  }

  return success;
}

/*
xt_core_bool_t talk_all_identities(ta_system_t *system, char *text)
{
  assert(system);
  assert(text);
  ta_identity_t *identity;
  ta_phrase_t *phrase;
  xt_core_bool_t success = xt_core_bool_true;

  xt_case_shardset_iterate_start(system->identities);
  while ((identity = xt_case_shardset_iterate_next(system->identities))) {
    phrase = ta_phrase_create(TA_PHRASE_NO_ID, text, strlen(text));
    if (phrase) {
      if (!ta_identity_talk(identity, phrase)) {
        xt_core_log_trace(system->log, " ta ", "ta_identity_talk");
        ta_phrase_destroy(phrase);
        success = xt_core_bool_false;
        break;
      }
    } else {
      xt_core_log_trace(system->log, " ta ", "ta_phrase_create");
      success = xt_core_bool_false;
      break;
    }
  }

  return success;
}
*/
