#ifndef ta_phrase_h
#define ta_phrase_h

#define TA_PHRASE_NO_ID 0
#define TA_PHRASE_TEXT_MAX_LENGTH 256

struct ta_phrase_t;
typedef struct ta_phrase_t ta_phrase_t;

int ta_phrase_compare(void *phrase_object_a, void *phrase_object_b);

void *ta_phrase_copy(void *phrase_object);

ta_phrase_t *ta_phrase_create(unsigned long id, char *text,
    unsigned short text_length);

void ta_phrase_destroy(void *phrase_object);

unsigned long ta_phrase_get_id(ta_phrase_t *phrase);

char *ta_phrase_get_text(ta_phrase_t *phrase);

void ta_phrase_set_id(ta_phrase_t *phrase, unsigned long id);

#endif
