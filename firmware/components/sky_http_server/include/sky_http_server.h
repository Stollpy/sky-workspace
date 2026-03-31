#pragma once

#include "esp_err.h"
#include "esp_http_server.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Route definition ──────────────────────────────────────────────── */

typedef esp_err_t (*sky_http_handler_t)(httpd_req_t *req);

typedef struct {
    const char          *uri;
    httpd_method_t       method;
    sky_http_handler_t   handler;
    void                *user_ctx;
} sky_http_route_t;

/* ── Configuration ─────────────────────────────────────────────────── */

typedef struct {
    uint16_t                 port;
    size_t                   max_uri_handlers;
    size_t                   stack_size;
    const sky_http_route_t  *routes;
    size_t                   route_count;
} sky_http_server_config_t;

#define SKY_HTTP_SERVER_CONFIG_DEFAULT() {                                \
    .port             = CONFIG_SKY_HTTP_SERVER_PORT,                      \
    .max_uri_handlers = CONFIG_SKY_HTTP_SERVER_MAX_URI_HANDLERS,          \
    .stack_size       = CONFIG_SKY_HTTP_SERVER_STACK_SIZE,                \
    .routes           = NULL,                                             \
    .route_count      = 0,                                                \
}

/* ── API ───────────────────────────────────────────────────────────── */

esp_err_t sky_http_server_init(const sky_http_server_config_t *config);
esp_err_t sky_http_server_start(void);
esp_err_t sky_http_server_stop(void);
void      sky_http_server_deinit(void);

esp_err_t sky_http_server_add_route(const sky_http_route_t *route);

#ifdef __cplusplus
}
#endif
