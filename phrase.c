#include "ta/phrase.h"
#include "xt/core/standard.h"
#include "xt/core/tools.h"
#include "xt/core/basic/unsigned_long.h"

struct ta_phrase_t {
  unsigned long id;
  char *text;
  unsigned short text_length;
};

int ta_phrase_compare(void *phrase_object_a, void *phrase_object_b)
{
  assert(phrase_object_a);
  assert(phrase_object_b);
  ta_phrase_t *phrase_a = phrase_object_a;
  ta_phrase_t *phrase_b = phrase_object_b;

  return xt_core_basic_unsigned_long_compare(&phrase_a->id, &phrase_b->id);
}

void *ta_phrase_copy(void *phrase_object)
{
  assert(phrase_object);
  ta_phrase_t *phrase = phrase_object;
  ta_phrase_t *phrase_copy;

  phrase_copy = ta_phrase_create(phrase->id, phrase->text,
      phrase->text_length);
  if (!phrase_copy) {
    xt_core_trace("ta_phrase_create");
  }

  return phrase_copy;
}

ta_phrase_t *ta_phrase_create(unsigned long id, char *text,
    unsigned short text_length)
{
  assert(text);
  assert(text_length <= TA_PHRASE_TEXT_MAX_LENGTH);
  ta_phrase_t *phrase;

  phrase = malloc(sizeof *phrase);
  if (phrase) {
    phrase->id = id;
    phrase->text_length = text_length;
    phrase->text = malloc(text_length + 1);
    if (phrase->text) {
      strncpy(phrase->text, text, text_length);
      *(phrase->text + text_length) = '\0';
    } else {
      xt_core_trace("malloc");
      free(phrase);
      phrase = NULL;
    }
  } else {
    xt_core_trace("malloc");
  }

  return phrase;
}

void ta_phrase_destroy(void *phrase_object)
{
  assert(phrase_object);
  ta_phrase_t *phrase = phrase_object;

  free(phrase->text);
  free(phrase);
}

unsigned long ta_phrase_get_id(ta_phrase_t *phrase)
{
  return phrase->id;
}

char *ta_phrase_get_text(ta_phrase_t *phrase)
{
  return phrase->text;
}

void ta_phrase_set_id(ta_phrase_t *phrase, unsigned long id)
{
  phrase->id = id;
}
