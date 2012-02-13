#include "ta/server.h"

void ta_server_free(ta_server_t *server) {}

void ta_server_init(ta_server_t *server, char *atom_directory,
    ta_system_t *ta_system, xt_core_log_t *log)
{
  assert(server);
  assert(atom_directory);
  assert(ta_system);
  assert(log);

  server->atom_directory = atom_directory;
  server->ta_system = ta_system;
  server->log = log;
}
