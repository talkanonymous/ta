#include "ta/identity.h"
#include "ta/server.h"
#include "ta/system.h"
#include "ta/http/engine.h"
#include "xt/case/map.h"
#include "xt/core/object.h"
#include "xt/core/engine.h"
#include "xt/core/string.h"
#include "xt/core/basic/void.h"
#include "xt/file/basic.h"
#include "xt/net/engine/thread.h"
#include "xt/net/http/message.h"

struct ta_http_engine_t {
  xt_net_server_system_t *net_server;
  ta_server_t *ta_server;
  ta_system_t *ta_system;
  xt_case_map_t *handlers;
  xt_core_iobject_t string_iobject;
  xt_core_iobject_t void_iobject;
  xt_core_log_t *log;
};

static char *construct_file_path(ta_server_t *ta_server, const char *filename);

static xt_net_message_status_t handle_request_javascript(void *engine_object,
    void *message_object);

static xt_net_message_status_t handle_request_javascript_redact
(void *engine_object, void *message_object);

static xt_net_message_status_t handle_request_listen(void *engine_object,
    void *message_object);

static xt_net_message_status_t handle_request_root(void *engine_object,
    void *message_object);

static xt_net_message_status_t handle_request_style_comic(void *engine_object,
    void *message_object);

static xt_net_message_status_t handle_request_style_hacker(void *engine_object,
    void *message_object);

static xt_net_message_status_t handle_request_style_hipster
(void *engine_object, void *message_object);

static xt_net_message_status_t handle_request_talk(void *engine_object,
    void *message_object);

static xt_net_message_status_t handle_static_file(void *engine_object,
    void *message_object, const char *filename, xt_core_content_t mime_type);

static xt_core_bool_t register_handlers(ta_http_engine_t *engine);

static xt_core_bool_t send_response_message(char *body,
    unsigned long body_size, xt_net_server_system_t *net_server,
    int client_socket, xt_core_log_t *log);

char *construct_file_path(ta_server_t *ta_server, const char *filename)
{
  assert(ta_server);
  assert(filename);
  char *file_path;

  file_path = xt_core_string_append(NULL, ta_server->atom_directory);
  if (file_path) {
    file_path = xt_core_string_append(file_path, "/");
    if (file_path) {
      file_path = xt_core_string_append(file_path, filename);
      if (!file_path) {
        xt_core_log_trace(ta_server->log, " ta ", "xt_core_string_append");
      }
    } else {
      xt_core_log_trace(ta_server->log, " ta ", "xt_core_string_append");
    }
  } else {
    xt_core_log_trace(ta_server->log, " ta ", "xt_core_string_append");
  }

  return file_path;
}

void *ta_http_engine_create(xt_net_server_system_t *net_server,
    void *custom_server_object)
{
  assert(net_server);
  assert(custom_server_object);
  ta_http_engine_t *engine;
  xt_core_bool_t so_far_so_good;

  engine = malloc(sizeof *engine);
  if (engine) {
    engine->net_server = net_server;
    engine->ta_server = custom_server_object;
    engine->ta_system = engine->ta_server->ta_system;

    /*  engine->log = ta_system_get_log(engine->focus_system);  */
    engine->log = engine->ta_server->log;

    xt_core_string_init_iobject(&engine->string_iobject);
    xt_core_basic_void_init_iobject(&engine->void_iobject);
    engine->handlers = xt_case_map_create(&engine->string_iobject,
        &engine->void_iobject, XT_CASE_MAP_DONT_DESTROY);
    if (engine->handlers) {
      so_far_so_good = xt_core_bool_true;
    } else {
      so_far_so_good = xt_core_bool_false;
      xt_core_log_trace(engine->log, " ta ", "xt_case_map_create");
    }
  } else {
    so_far_so_good = xt_core_bool_false;
    xt_core_trace("malloc");
  }

  if (!register_handlers(engine)) {
    so_far_so_good = xt_core_bool_false;
    xt_core_trace("register_handlers");
  }

  if (!so_far_so_good && engine) {
    if (engine->handlers) {
      xt_case_map_destroy(engine->handlers);
    }
    free(engine);
    engine = NULL;
  }

  return engine;
}

void ta_http_engine_destroy(void *engine_object)
{
  assert(engine_object);
  ta_http_engine_t *engine;

  engine = engine_object;
  xt_case_map_destroy(engine->handlers);
  free(engine);
}

xt_net_server_system_handle_message_f ta_http_engine_get_handler_for_message
(void *engine_object, void *message_object)
{
  assert(engine_object);
  assert(message_object);
  ta_http_engine_t *engine = engine_object;
  xt_net_server_system_handle_message_f handler;
  xt_net_http_message_t *http_message = message_object;
  char *resource_name;

  resource_name = xt_net_http_message_get_resource_name(http_message);

  xt_core_log_enter(engine->log, " ta ", "%s", resource_name);

  if ((strlen(resource_name) >= 3)
      && (0 == strncmp("/be", resource_name, 3))) {
    handler = handle_request_root;
  } else {
    handler = xt_case_map_find(engine->handlers, resource_name);
    if (!handler) {
      handler = handle_request_root;
      xt_core_trace("xt_case_map_find");
    }
  }

  return handler;
}

void ta_http_engine_maintain(void *engine_object)
{
  assert(engine_object);
  ta_http_engine_t *engine = engine_object;

  ta_system_maintain(engine->ta_system);
  xt_net_server_system_print_stats(engine->net_server);
}

void ta_http_engine_run(void *engine_thread_object)
{
  assert(engine_thread_object);
  xt_net_engine_thread_t *engine_thread;
  ta_http_engine_t *engine;
  unsigned char thread_index;

  engine_thread = engine_thread_object;
  engine = engine_thread->engine_object;
  thread_index = engine_thread->thread_index;

  xt_net_server_system_process_messages(engine->net_server, XT_NET_ENGINE_HTTP,
      thread_index);
}

void ta_http_engine_start(void *engine_thread_object)
{
  assert(engine_thread_object);
  xt_net_engine_thread_t *engine_thread;
  ta_http_engine_t *engine;

  engine_thread = engine_thread_object;
  engine = engine_thread->engine_object;

  xt_core_log_enter(engine->log, " ta ", "http engine thread %i start",
      engine_thread->thread_index);
}

void ta_http_engine_stop(void *engine_thread_object)
{
  assert(engine_thread_object);
  xt_net_engine_thread_t *engine_thread;
  ta_http_engine_t *engine;

  engine_thread = engine_thread_object;
  engine = engine_thread->engine_object;

  xt_core_log_enter(engine->log, " ta ", "http engine thread %i stop",
      engine_thread->thread_index);
}

xt_net_message_status_t handle_request_javascript(void *engine_object,
    void *message_object)
{
  return handle_static_file(engine_object, message_object, "ta.js",
      XT_CORE_CONTENT_MIME_TEXT_JAVASCRIPT);
}

xt_net_message_status_t handle_request_javascript_redact(void *engine_object,
    void *message_object)
{
  return handle_static_file(engine_object, message_object,
      "redact/js/redact.js", XT_CORE_CONTENT_MIME_TEXT_JAVASCRIPT);
}

/*
  TODO: clean up the structure of this, with regard to error-checking
*/
xt_net_message_status_t handle_request_listen(void *engine_object,
    void *message_object)
{
  assert(engine_object);
  assert(message_object);
  xt_net_message_status_t message_status;
  ta_http_engine_t *engine;
  xt_net_http_message_t *message;
  char *body = NULL;
  int client_socket;
  unsigned long next_phrase_id;
  xt_case_list_t *phrases;
  ta_phrase_t *phrase;
#define PHRASE_STRING_SIZE 10 + 1 + TA_PHRASE_TEXT_MAX_LENGTH + 1 + 1
  char phrase_string[PHRASE_STRING_SIZE];
  xt_core_bool_t ok = xt_core_bool_true;
  char *identity_name;

  engine = engine_object;
  message = message_object;

  message_status = XT_NET_MESSAGE_STATUS_CANT_HANDLE;

  identity_name = xt_net_http_message_get_uri_parameter
    (message, "identity_name");
  if (identity_name) {
    if (xt_net_http_message_get_uri_parameter_as_unsigned_long
        (message, "next_phrase_id", &next_phrase_id)) {
      xt_core_log_enter(engine->log, " ta " , "/listen::%s::%lu",
          identity_name, next_phrase_id);
      phrases = ta_system_listen(engine->ta_system, identity_name,
          next_phrase_id);
      if (phrases) {
        if (xt_case_list_get_size(phrases) > 0) {
          /*  phrase_string[PHRASE_STRING_SIZE] = '\0';  */
          xt_case_list_iterate_start(phrases);
          while ((phrase = xt_case_list_iterate_next(phrases))) {
            if (snprintf(phrase_string, PHRASE_STRING_SIZE, "%lu\t%s\n",
                    ta_phrase_get_id(phrase),
                    ta_phrase_get_text(phrase)) >= 0) {
              body = xt_core_string_append(body, phrase_string);
              if (!body) {
                xt_core_log_trace(engine->log, " ta ",
                    "xt_core_string_append");
                ok = xt_core_bool_false;
              }
            } else {
              xt_core_log_trace(engine->log, " ta ", "sprintf");
              ok = xt_core_bool_false;
            }
          }
        } else {
          body = xt_core_string_append(body,
              "status:ok_nothing_to_listen_to\n");
        }
        xt_case_list_destroy(phrases);
      } else {
        xt_core_log_trace(engine->log, " ta ", "ta_system_listen");
        ok = xt_core_bool_false;
      }
      if (!ok) {
        if (body) {
          free(body);
          body = NULL;
        }
        body = xt_core_string_append(body, "status:nok\n");
        if (!body) {
          xt_core_log_trace(engine->log, " ta ", "xt_core_string_append");
        }
      }
    } else {
      xt_core_log_trace(engine->log, " ta ",
          "xt_net_http_message_get_uri_parameter_as_unsigned_long");
      ok = xt_core_bool_false;
      body = xt_core_string_append(body,
          "status:nok_missing_next_phrase_id\n");
      if (!body) {
        xt_core_log_trace(engine->log, " ta ", "xt_core_string_append");
      }
    }
  } else {
    xt_core_log_trace(engine->log, " ta ",
        "xt_net_http_message_get_uri_parameter");
      ok = xt_core_bool_false;
      body = xt_core_string_append(body,
          "status:nok_missing_identity_name\n");
      if (!body) {
        xt_core_log_trace(engine->log, " ta ", "xt_core_string_append");
      }
  }

  if (body) {
    client_socket = xt_net_http_message_get_client_socket(message);
    if (send_response_message(body, strlen(body), engine->net_server,
            client_socket, engine->log)) {
      message_status = XT_NET_MESSAGE_STATUS_HANDLED;
    } else {
      xt_core_log_trace(engine->log, " ta ", "send_response_message");
    }
    free(body);
  } else {
    xt_core_log_trace(engine->log, " ta ", "ughh..");
  }

  return message_status;
}

xt_net_message_status_t handle_request_root(void *engine_object,
    void *message_object)
{
  return handle_static_file(engine_object, message_object, "ta.html",
      XT_CORE_CONTENT_MIME_TEXT_HTML);
}

xt_net_message_status_t handle_request_style_comic(void *engine_object,
    void *message_object)
{
  return handle_static_file(engine_object, message_object, "comic.css",
      XT_CORE_CONTENT_MIME_TEXT_CSS);
}

xt_net_message_status_t handle_request_style_hacker(void *engine_object,
    void *message_object)
{
  return handle_static_file(engine_object, message_object, "hacker.css",
      XT_CORE_CONTENT_MIME_TEXT_CSS);
}

xt_net_message_status_t handle_request_style_hipster(void *engine_object,
    void *message_object)
{
  return handle_static_file(engine_object, message_object, "hipster.css",
      XT_CORE_CONTENT_MIME_TEXT_CSS);
}

/*
  TODO: restructure; there are definite leaks in here
*/
xt_net_message_status_t handle_request_talk(void *engine_object,
    void *message_object)
{
  assert(engine_object);
  assert(message_object);
  xt_net_message_status_t message_status;
  ta_http_engine_t *engine;
  xt_net_http_message_t *message;
  char *body;
  int client_socket;
  /*  char *identity;  */
  char *text;
  unsigned short text_length;
  ta_phrase_t *phrase;
  xt_core_bool_t talk_ok = xt_core_bool_false;
  char *identity_name;

  engine = engine_object;
  message = message_object;

  message_status = XT_NET_MESSAGE_STATUS_CANT_HANDLE;

  identity_name = xt_net_http_message_get_uri_parameter
    (message, "identity_name");
  if (identity_name) {
    text = xt_net_http_message_get_uri_parameter(message, "text");
    if (text) {
      text_length = strlen(text);
      if (text_length > TA_PHRASE_TEXT_MAX_LENGTH) {
        *(text + TA_PHRASE_TEXT_MAX_LENGTH) = '\0';
        text_length = TA_PHRASE_TEXT_MAX_LENGTH;
      }
      phrase = ta_phrase_create(TA_PHRASE_NO_ID, text, text_length);
      if (phrase) {
        xt_core_log_enter(engine->log, " ta " , "/talk::%s::%s",
            identity_name, text);
        if (ta_system_talk(engine->ta_system, identity_name, phrase)) {
          talk_ok = xt_core_bool_true;
        } else {
          xt_core_log_trace(engine->log, " ta ", "ta_system_talk");
        }
      } else {
        xt_core_log_trace(engine->log, " ta ", "ta_phrase_create");
      }
      if (talk_ok) {
        body = xt_core_string_append(NULL, "status:ok\n");
      } else {
        body = xt_core_string_append(NULL, "status:nok\n");
      }
    } else {
      body = xt_core_string_append(NULL, "status:nok_missing_text\n");
    }
  } else {
    body = xt_core_string_append(NULL, "status:nok_missing_identity_name\n");
  }


  if (body) {
    client_socket = xt_net_http_message_get_client_socket(message);
    if (send_response_message(body, strlen(body), engine->net_server,
            client_socket, engine->log)) {
      message_status = XT_NET_MESSAGE_STATUS_HANDLED;
    } else {
      xt_core_log_trace(engine->log, " ta ", "send_response_message");
    }
    free(body);
  } else {
    xt_core_log_trace(engine->log, " ta ", "xt_core_string_append");
  }

  return message_status;
}

xt_net_message_status_t handle_static_file(void *engine_object,
    void *message_object, const char *filename, xt_core_content_t mime_type)
{
  assert(engine_object);
  assert(message_object);
  ta_http_engine_t *engine;
  xt_net_http_message_t *message;
  xt_net_http_message_t *response_message;
  int client_socket;
  char *body;
  unsigned long body_size;
  xt_file_basic_t *file;
  char *file_path;

  engine = engine_object;
  message = message_object;

  client_socket = xt_core_message_get_client_socket(message);

  file_path = construct_file_path(engine->ta_server, filename);
  if (file_path) {
    file = xt_file_basic_create(file_path, XT_FILE_MODE_OPEN_FOR_READ);
    if (file) {
      if (xt_file_basic_get_as_blob(file, &body, &body_size)) {
        response_message = xt_net_http_message_create
          (client_socket, XT_NET_HTTP_METHOD_UNKNOWN, XT_NET_HTTP_STATUS_OK,
              XT_NET_HTTP_MESSAGE_NO_RESOURCE_PATH, XT_NET_HTTP_VERSION_1_0,
              XT_NET_HTTP_MESSAGE_NO_HEADERS);
        if (response_message) {
          xt_net_http_message_set_body
            (response_message, mime_type, body, body_size);
          if (!xt_net_server_system_send_message
              (engine->net_server, response_message)) {
            xt_core_log_trace(engine->log, " ta ",
                "xt_net_server_send_message");
          }
        } else {
          xt_core_log_trace(engine->log, " ta ", "xt_net_http_message_create");
        }
        free(body);
      } else {
        xt_core_log_trace(engine->log, " ta ", "xt_file_basic_get_as_blob");
      }
      xt_file_basic_destroy(file);
    } else {
      xt_core_log_trace(engine->log, " ta ", "xt_file_basic_create");
    }
    free(file_path);
  } else {
    xt_core_log_trace(engine->log, " ta ", "construct_file_path");
  }

  return XT_NET_MESSAGE_STATUS_HANDLED;
}

xt_core_bool_t register_handlers(ta_http_engine_t *engine)
{
  assert(engine);
  xt_core_bool_t success;

  success = xt_core_bool_true;

  if (success && !xt_case_map_add(engine->handlers, "/",
          handle_request_root)) {
    success = xt_core_bool_false;
    xt_core_log_trace(engine->log, " ta ", "xt_case_map_add");
  }

  if (success && !xt_case_map_add(engine->handlers, "/listen",
          handle_request_listen)) {
    success = xt_core_bool_false;
    xt_core_log_trace(engine->log, " ta ", "xt_case_map_add");
  }

  if (success && !xt_case_map_add(engine->handlers, "/comic.css",
          handle_request_style_comic)) {
    success = xt_core_bool_false;
    xt_core_log_trace(engine->log, " ta ", "xt_case_map_add");
  }

  if (success && !xt_case_map_add(engine->handlers, "/hacker.css",
          handle_request_style_hacker)) {
    success = xt_core_bool_false;
    xt_core_log_trace(engine->log, " ta ", "xt_case_map_add");
  }

  if (success && !xt_case_map_add(engine->handlers, "/hipster.css",
          handle_request_style_hipster)) {
    success = xt_core_bool_false;
    xt_core_log_trace(engine->log, " ta ", "xt_case_map_add");
  }

  if (success && !xt_case_map_add(engine->handlers, "/redact.js",
          handle_request_javascript_redact)) {
    success = xt_core_bool_false;
    xt_core_log_trace(engine->log, " ta ", "xt_case_map_add");
  }

  if (success && !xt_case_map_add(engine->handlers, "/talk",
          handle_request_talk)) {
    success = xt_core_bool_false;
    xt_core_log_trace(engine->log, " ta ", "xt_case_map_add");
  }

  if (success && !xt_case_map_add(engine->handlers, "/ta.js",
          handle_request_javascript)) {
    success = xt_core_bool_false;
    xt_core_log_trace(engine->log, " ta ", "xt_case_map_add");
  }

  return success;
}

xt_core_bool_t send_response_message(char *body, unsigned long body_size,
    xt_net_server_system_t *net_server, int client_socket, xt_core_log_t *log)
{
  assert(body);
  assert(net_server);
  xt_core_bool_t success;
  xt_net_http_message_t *response_message;

  success = xt_core_bool_false;

  response_message = xt_net_http_message_create(client_socket,
      XT_NET_HTTP_METHOD_UNKNOWN, XT_NET_HTTP_STATUS_OK,
      XT_NET_HTTP_MESSAGE_NO_RESOURCE_PATH, XT_NET_HTTP_VERSION_1_0,
      XT_NET_HTTP_MESSAGE_NO_HEADERS);
  if (response_message) {
    xt_net_http_message_set_body(response_message,
        XT_CORE_CONTENT_MIME_TEXT_HTML, body, body_size);
    if (xt_net_server_system_send_message(net_server, response_message)) {
      success = xt_core_bool_true;
    } else {
      xt_net_http_message_destroy(response_message);
      xt_core_log_trace(log, " ta ", "xt_net_server_send_message");
    }
  } else {
    xt_core_log_trace(log, " ta ", "xt_net_http_message_create");
  }

  return success;
}
