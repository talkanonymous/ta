#ifndef ta_server_h
#define ta_server_h

#include "ta/system.h"
#include "xt/core/log.h"

struct ta_server_t {
  char *atom_directory;
  ta_system_t *ta_system;
  xt_core_log_t *log;
};
typedef struct ta_server_t ta_server_t;

void ta_server_free(ta_server_t *server);

void ta_server_init(ta_server_t *server, char *atom_directory,
    ta_system_t *ta_system, xt_core_log_t *log);

#endif
