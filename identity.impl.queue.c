#include "ta/identity.h"
#include "xt/core/standard.h"
#include "xt/core/string.h"
#include "xt/core/tools.h"
#include "xt/sync/qutex.h"

#define MAX_PHRASES_SIZE (MAX_PHRASE_LAG * 3)

struct ta_identity_t {
  char *name;
  xt_case_list_t *phrases;
  xt_sync_qutex_t *phrases_qutex;
  xt_core_log_t *log;
};

static void check_range_of_next_phrase_id(unsigned long *next_phrase_id,
    unsigned long phrases_size);

void check_range_of_next_phrase_id(unsigned long *next_phrase_id,
    unsigned long phrases_size)
{
  assert(next_phrase_id);

  if (*next_phrase_id > phrases_size) {
    /*  printf("%lu::%lu\n", *next_phrase_id, phrases_size);  */
    if ((phrases_size > MAX_PHRASE_LAG)) {
      *next_phrase_id = phrases_size - MAX_PHRASE_LAG;
    } else {
      *next_phrase_id = 0;
    }
  }
  if ((phrases_size > MAX_PHRASE_LAG)
      && (*next_phrase_id < (phrases_size - MAX_PHRASE_LAG))) {
    *next_phrase_id = phrases_size - MAX_PHRASE_LAG;
  }
}

void ta_identity_clear(ta_identity_t *identity) {}

int ta_identity_compare(void *identity_object_a, void *identity_object_b)
{
  assert(identity_object_a);
  assert(identity_object_b);
  ta_identity_t *identity_a = identity_object_a;
  ta_identity_t *identity_b = identity_object_b;

  return xt_core_string_compare(identity_a->name, identity_b->name);
}

xt_core_bool_t ta_identity_compare_equal(void *identity_object_a,
    void *identity_object_b)
{
  assert(identity_object_a);
  assert(identity_object_b);
  ta_identity_t *identity_a = identity_object_a;
  ta_identity_t *identity_b = identity_object_b;

  return xt_core_string_compare_equal(identity_a->name, identity_b->name);
}

void *ta_identity_copy(void *identity_object)
{
  assert(identity_object);
  xt_core_trace_exit("TODO: implement");
  return NULL;
}

ta_identity_t *ta_identity_create(char *name, xt_core_log_t *log)
{
  assert(name);
  assert(log);
  ta_identity_t *identity;
  xt_core_bool_t so_far_so_good;

  identity = malloc(sizeof *identity);
  if (identity) {
    identity->log = log;
    identity->phrases_qutex = NULL;
    identity->phrases = NULL;
    identity->name = xt_core_string_copy(name);
    if (identity->name) {
      so_far_so_good = xt_core_bool_true;
    } else {
      so_far_so_good = xt_core_bool_false;
      xt_core_log_trace(log, " ta ", "xt_core_string_copy");
    }
  } else {
    so_far_so_good = xt_core_bool_false;
    xt_core_log_trace(log, " ta ", "malloc");
  }

  if (so_far_so_good) {
    identity->phrases = xt_case_list_create(ta_phrase_compare, ta_phrase_copy,
        ta_phrase_destroy);
    if (identity->phrases) {
      so_far_so_good = xt_core_bool_true;
      xt_case_list_set_size_limit(identity->phrases, MAX_PHRASES_SIZE);
    } else {
      so_far_so_good = xt_core_bool_false;
      xt_core_log_enter(log, " ta ", "xt_case_list_create");
    }
  }

  if (so_far_so_good) {
    identity->phrases_qutex = xt_sync_qutex_create();
    if (!identity->phrases_qutex) {
      so_far_so_good = xt_core_bool_false;
      xt_core_log_enter(log, " ta ", "xt_sync_qutex_create");
    }
  }

  if (identity && !so_far_so_good) {
    if (identity->name) {
      xt_core_string_destroy(identity->name);
    }
    if (identity->phrases) {
      xt_case_list_destroy(identity->phrases);
    }
    if (identity->phrases_qutex) {
      xt_sync_qutex_destroy(identity->phrases_qutex);
    }
    free(identity);
    identity = NULL;
  }

  return identity;
}

ta_identity_t *ta_identity_create_decoy(char *name, xt_core_log_t *log)
{
  assert(name);
  assert(log);
  ta_identity_t *identity;

  identity = malloc(sizeof *identity);
  if (identity) {
    identity->log = log;
    identity->name = xt_core_string_copy(name);
    if (!identity->name) {
      xt_core_log_trace(log, " ta ", "xt_core_string_copy");
      free(identity);
      identity = NULL;
    }
  } else {
    xt_core_log_trace(log, " ta ", "malloc");
  }

  return identity;
}

void ta_identity_destroy(void *identity_object)
{
  assert(identity_object);
  ta_identity_t *identity = identity_object;

  xt_core_string_destroy(identity->name);
  xt_case_list_destroy(identity->phrases);
  xt_sync_qutex_destroy(identity->phrases_qutex);
  free(identity);
}

void ta_identity_destroy_decoy(void *identity_decoy_object)
{
  assert(identity_decoy_object);
  ta_identity_t *identity = identity_decoy_object;

  xt_core_string_destroy(identity->name);
  free(identity);
}

unsigned long ta_identity_get_size(ta_identity_t *identity)
{
  return xt_case_list_get_size(identity->phrases);
}

unsigned long ta_identity_hash(void *identity_object)
{
  assert(identity_object);
  ta_identity_t *identity = identity_object;
  return xt_core_string_hash(identity->name);
}

xt_case_list_t *ta_identity_listen(ta_identity_t *identity,
    unsigned long next_phrase_id)
{
  assert(identity);
  unsigned long i;
  xt_case_list_t *phrases;
  ta_phrase_t *phrase;
  ta_phrase_t *phrase_copy;
  unsigned long phrases_size;

  phrases = xt_case_list_create(ta_phrase_compare, ta_phrase_copy,
      ta_phrase_destroy);
  if (phrases) {
    xt_sync_qutex_lock_shared(identity->phrases_qutex);
    phrases_size = xt_case_list_get_size(identity->phrases);
    check_range_of_next_phrase_id(&next_phrase_id, phrases_size);
    for (i = next_phrase_id; i < phrases_size; i++) {
      phrase = xt_case_list_find_at(identity->phrases, i);
      assert(phrase);
      phrase_copy = ta_phrase_copy(phrase);
      if (phrase_copy) {
        if (!xt_case_list_add_last(phrases, phrase_copy)) {
          xt_core_log_trace(identity->log, " ta ", "xt_case_list_add_last");
          ta_phrase_destroy(phrase_copy);
          break;
        }
      } else {
        xt_core_log_trace(identity->log, " ta ", "ta_phrase_copy");
        break;
      }
    }
    xt_sync_qutex_unlock_shared(identity->phrases_qutex);
  } else {
    xt_core_log_trace(identity->log, " ta ", "xt_case_list_create");
  }

  return phrases;
}

unsigned long ta_identity_mod(void *identity_object, unsigned long divisor)
{
  assert(identity_object);
  ta_identity_t *identity = identity_object;
  return xt_core_string_mod(identity->name, divisor);
}

xt_core_bool_t ta_identity_talk(ta_identity_t *identity, ta_phrase_t *phrase)
{
  assert(identity);
  assert(phrase);
  xt_core_bool_t success;

  xt_sync_qutex_lock_exclusive(identity->phrases_qutex);
  ta_phrase_set_id(phrase, xt_case_list_get_size(identity->phrases));
  if (xt_case_list_add_last(identity->phrases, phrase)) {
    xt_sync_qutex_unlock_exclusive(identity->phrases_qutex);
    success = xt_core_bool_true;
  } else {
    xt_sync_qutex_unlock_exclusive(identity->phrases_qutex);
    xt_core_log_enter(identity->log, " ta ", "xt_case_list_add_last");
    success = xt_core_bool_false;
  }

  return success;
}
