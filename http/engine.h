#ifndef ta_http_engine_h
#define ta_http_engine_h

#include "xt/net/server/system.h"
#include "xt/net/tools.h"

struct ta_http_engine_t;
typedef struct ta_http_engine_t ta_http_engine_t;

void *ta_http_engine_create(xt_net_server_system_t *net_server,
    void *custom_server_object);

void ta_http_engine_destroy(void *engine_object);

xt_net_server_system_handle_message_f ta_http_engine_get_handler_for_message
(void *engine_object, void *message_object);

void ta_http_engine_maintain(void *engine_object);

void ta_http_engine_run(void *engine_thread_object);

void ta_http_engine_start(void *engine_thread_object);

void ta_http_engine_stop(void *engine_thread_object);

#endif
