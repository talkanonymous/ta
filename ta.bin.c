#include "ta/server.h"
#include "ta/system.h"
#include "ta/tools.h"
#include "ta/http/engine.h"
#include "xt/case/set.h"
#include "xt/core/objects.h"
#include "xt/core/standard.h"
#include "xt/file/basic.h"
#include "xt/net/http/message.h"
#include "xt/net/http/post.h"

#define CONF_FILENAME "ta.conf"
#define DEFAULT_ATOM_DIRECTORY "."
#define DEFAULT_CLEAR_ALL_IDENTITIES_PERIOD_MINUTES 60
#define DEFAULT_IDENTITIES_SHARD_COUNT 1024
#define DEFAULT_HTTP_SERVER_MIN_PORT 5080
#define DEFAULT_HTTP_SERVER_MAX_PORT 5089
#define HTTP_SERVER_MAX_THREADS 16
#define LOG_FILENAME "ta.out"

int main(int argc, char *argv[])
{
  ta_system_t *ta_system;
  ta_server_t ta_server;
  xt_net_server_system_t *http_server;
  unsigned short http_server_min_port;
  unsigned short http_server_max_port;
  xt_net_engine_iengine_t http_iengine;
  xt_core_imessage_t http_imessage;
  xt_net_post_ipost_t http_ipost;
  xt_core_objects_t objects;
  xt_file_basic_t *log_file;
  xt_core_log_t *log;
  FILE *log_file_file;
  xt_config_system_t *config;
  char *atom_directory;
  unsigned long identities_shard_count;
  unsigned short clear_all_identities_period_minutes;

  xt_core_objects_init(&objects);

  log_file = xt_file_basic_create
    (LOG_FILENAME, XT_FILE_MODE_TRUNCATE_OR_CREATE_FOR_WRITE);
  if (!log_file) {
    xt_core_trace_exit("xt_file_basic_create");
  }

  log = xt_core_log_create(stdout);
  if (!log) {
    xt_core_trace_exit("xt_core_log_create");
  }

  log_file_file = xt_file_basic_get_file(log_file);
  if (log_file_file) {
    if (!xt_core_log_add_file(log, log_file_file)) {
      xt_core_log_trace_exit(log, " ta ", "xt_core_log_add_file");
    }
  } else {
    xt_core_log_trace_exit(log, " ta ", "xt_file_basic_get_file");
  }

#ifdef XT_BUILD_DEMO
  xt_core_log_enter(log, " ta ", "demo build");
#endif
#ifdef XT_BUILD_DEVELOPMENT
  xt_core_log_enter(log, " ta ", "development build");
#endif
#ifdef XT_BUILD_RELEASE
  xt_core_log_enter(log, " ta ", "release build");
#endif

  config = xt_config_system_create(argc, argv, CONF_FILENAME, &objects);
  if (!config) {
    xt_core_log_trace_exit(log, " ta ", "xt_config_system_create");
  }

  xt_config_system_find_as_string(config, "atom_directory", &atom_directory,
      DEFAULT_ATOM_DIRECTORY);
  xt_core_log_enter(log, " ta ", "using atom directory %s", atom_directory);

  xt_config_system_find_as_unsigned_short(config, "http_server_min_port",
      &http_server_min_port, DEFAULT_HTTP_SERVER_MIN_PORT);
  xt_config_system_find_as_unsigned_short(config, "http_server_max_port",
      &http_server_max_port, DEFAULT_HTTP_SERVER_MAX_PORT);
  xt_core_log_enter(log, " ta ", "using http server port range %i..%i",
      http_server_min_port, http_server_max_port);

  xt_config_system_find_as_unsigned_long(config, "identities_shard_count",
      &identities_shard_count, DEFAULT_IDENTITIES_SHARD_COUNT);
  xt_core_log_enter(log, " ta ", "identities_shard_count is %i",
      identities_shard_count);

  xt_config_system_find_as_unsigned_short(config,
      "clear_all_identities_period_minutes",
      &clear_all_identities_period_minutes,
      DEFAULT_CLEAR_ALL_IDENTITIES_PERIOD_MINUTES);
  xt_core_log_enter(log, " ta ", "clear_all_identities_period_minutes is %i",
      clear_all_identities_period_minutes);

  xt_net_engine_iengine_init(&http_iengine, ta_http_engine_create,
      ta_http_engine_destroy, ta_http_engine_get_handler_for_message,
      ta_http_engine_maintain, ta_http_engine_run, ta_http_engine_start,
      ta_http_engine_stop);

  xt_core_imessage_init(&http_imessage, xt_net_http_message_destroy,
      xt_net_http_message_get_client_socket, xt_net_http_message_get_engine_id,
      xt_net_http_message_get_type);

  xt_net_post_ipost_init(&http_ipost, xt_net_http_post_compare,
      xt_net_http_post_create, xt_net_http_post_create_decoy,
      xt_net_http_post_destroy, xt_net_http_post_destroy_decoy,
      xt_net_http_post_get_last_receive_activity_time,
      xt_net_http_post_get_socket, xt_net_http_post_get_stats,
      xt_net_http_post_receive_message, xt_net_http_post_receive_messages,
      xt_net_http_post_send_message, xt_net_http_post_send_messages,
      xt_net_http_post_is_socket_closed, xt_net_http_post_mod,
      xt_net_http_post_compare_equal);

  ta_system = ta_system_create(identities_shard_count,
      clear_all_identities_period_minutes, log);
  if (!ta_system) {
    xt_core_log_trace_exit(log, " ta ", "ta_system_create");
  }

  ta_server_init(&ta_server, atom_directory, ta_system, log);

  http_server = xt_net_server_system_create("ta", http_server_min_port,
      http_server_max_port, HTTP_SERVER_MAX_THREADS, &http_imessage,
      &http_ipost, ta_get_engine_name, XT_NET_SERVER_SYSTEM_NO_CONFIG_SYSTEM,
      log);
  if (!http_server) {
    xt_core_log_trace_exit(log, " ta ", "xt_net_server_system_create");
  }

  if (!xt_net_server_system_register_engine(http_server, XT_NET_ENGINE_HTTP,
          &ta_server, &http_iengine, 4, 8, XT_NET_MAINTAIN_1_MINUTE,
          XT_NET_MESSAGE_TYPE_COUNT_NONE)) {
    xt_core_log_trace_exit(log, " ta ", "xt_net_server_register_engine");
  }

  if (!xt_net_server_system_start(http_server)) {
    xt_core_log_trace_exit(log, " ta ", "xt_net_server_start");
  }

  xt_net_server_system_destroy(http_server);
  ta_server_free(&ta_server);
  ta_system_destroy(ta_system);
  xt_file_basic_destroy(log_file);
  xt_config_system_destroy(config);
  xt_core_log_destroy(log);

  return 0;
}
