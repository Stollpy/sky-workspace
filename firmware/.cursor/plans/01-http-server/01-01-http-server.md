# Plan 01 — HTTP Server : Phase 1 — Component Shell
## Objectif
Créer le composant `sky_http_server` : une coquille générique par-dessus `esp_http_server` qui permet à chaque firmware produit d'injecter ses propres routes HTTP via un tableau de structures déclaratives avec callbacks.
## Prérequis
- Plan 00, Phase 2 (Core Package) complétée
- `sky_wifi` fonctionnel (le serveur HTTP nécessite la stack TCP/IP)
## Architecture
```
┌──────────────────────────────────────────────┐
│             sky_http_server                   │
├──────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────┐ │
│  │        Route Table (config)             │ │
│  │  [ {uri, method, handler, ctx}, ... ]   │ │
│  └──────────────────┬──────────────────────┘ │
│                     │                        │
│  ┌──────────────────▼──────────────────────┐ │
│  │         esp_http_server (httpd)          │ │
│  │    register_uri_handler() par route     │ │
│  └──────────────────┬──────────────────────┘ │
│                     │                        │
│  ┌──────────────────▼──────────────────────┐ │
│  │         Service Lifecycle               │ │
│  │   init → start(httpd) → stop → deinit   │ │
│  └──────────────────┬──────────────────────┘ │
│                     │                        │
│  ┌──────────────────▼──────────────────────┐ │
│  │         sky_registry integration        │ │
│  └─────────────────────────────────────────┘ │
└──────────────────────────────────────────────┘
```
Le firmware produit (ex: Luxia) définit ses routes en dehors du composant et les passe à la config :
```
main.c / luxia_cmd.c
  │
  │  luxia_routes[] = { {"/api/status", GET, handle_status, NULL}, ... }
  │
  ▼
sky_http_server_init(&cfg)   ←── injecte les routes
  │
  ▼
sky_core_start()
  │
  ▼
httpd démarre, enregistre chaque route automatiquement
```
## Structure du composant
```
components/sky_http_server/
├── CMakeLists.txt
├── Kconfig
├── include/
│   └── sky_http_server.h
└── src/
    └── sky_http_server.c
```
## CMakeLists.txt
```cmake
idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "include"
    REQUIRES sky_core
    PRIV_REQUIRES esp_http_server
)
```
`esp_http_server` est un composant standard ESP-IDF — pas de dépendance externe.
## Kconfig
```kconfig
menu "Sky HTTP Server"
    config SKY_HTTP_SERVER_ENABLED
        bool "Enable HTTP Server"
        default y
        help
            Active le serveur HTTP Sky.
            Désactiver pour économiser de la flash/RAM
            si le firmware n'a pas besoin de servir du HTTP.
    config SKY_HTTP_SERVER_PORT
        int "HTTP port"
        default 80
        range 1 65535
        depends on SKY_HTTP_SERVER_ENABLED
    config SKY_HTTP_SERVER_MAX_URI_HANDLERS
        int "Maximum URI handlers"
        default 16
        range 4 64
        depends on SKY_HTTP_SERVER_ENABLED
    config SKY_HTTP_SERVER_STACK_SIZE
        int "Server task stack size (bytes)"
        default 4096
        range 2048 16384
        depends on SKY_HTTP_SERVER_ENABLED
endmenu
```
## Header publique
```c
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
```
### Notes sur le design de l'API
- **`sky_http_handler_t`** utilise la même signature que `esp_httpd_uri_handler_func_t` pour que les handlers restent idiomatiques ESP-IDF. Le `user_ctx` est passé via `httpd_req_t->user_ctx` automatiquement.
- **`add_route`** permet l'ajout dynamique post-init (ex: depuis `luxia_cmd_init()`). Les routes ajoutées alors que le serveur tourne sont enregistrées immédiatement sur le `httpd_handle_t`.
- **Pas de `sky_http_handler_t` custom avec `user_ctx` en paramètre** — on réutilise le mécanisme natif `httpd_req_t->user_ctx` d'ESP-IDF pour éviter une abstraction inutile.
## Implémentation
```c
#include "sky_http_server.h"
#include "sky_registry.h"
#include "esp_log.h"
#include <string.h>
static const char *TAG = "sky_http";
/* ── Module state ──────────────────────────────────────────────────── */
static struct {
    sky_http_server_config_t config;
    httpd_handle_t           handle;
    bool                     initialized;
    bool                     started;
    bool                     registered;
} s_http;
/* ── Internal: register a single route on httpd ────────────────────── */
static esp_err_t register_route(const sky_http_route_t *route)
{
    httpd_uri_t uri_def = {
        .uri      = route->uri,
        .method   = route->method,
        .handler  = route->handler,
        .user_ctx = route->user_ctx,
    };
    return httpd_register_uri_handler(s_http.handle, &uri_def);
}
/* ── Service lifecycle ─────────────────────────────────────────────── */
static esp_err_t svc_init(void *cfg)  { (void)cfg; return ESP_OK; }
static esp_err_t svc_start(void)      { return sky_http_server_start(); }
static esp_err_t svc_stop(void)       { return sky_http_server_stop(); }
static esp_err_t svc_deinit(void)     { sky_http_server_deinit(); return ESP_OK; }
static esp_err_t ensure_registered(void)
{
    if (s_http.registered) return ESP_OK;
    static const sky_service_lifecycle_t lc = {
        .init = svc_init, .start = svc_start,
        .stop = svc_stop, .deinit = svc_deinit,
    };
    esp_err_t err = sky_registry_register("sky_http", &lc, NULL);
    if (err != ESP_OK) return err;
    s_http.registered = true;
    return ESP_OK;
}
/* ── Public API ────────────────────────────────────────────────────── */
esp_err_t sky_http_server_init(const sky_http_server_config_t *config)
{
    if (s_http.initialized) return ESP_ERR_INVALID_STATE;
    s_http.config = config ? *config
                           : (sky_http_server_config_t)SKY_HTTP_SERVER_CONFIG_DEFAULT();
    s_http.handle      = NULL;
    s_http.initialized = true;
    ensure_registered();
    ESP_LOGI(TAG, "initialized (port=%d, max_routes=%d)",
             s_http.config.port, (int)s_http.config.max_uri_handlers);
    return ESP_OK;
}
esp_err_t sky_http_server_start(void)
{
    if (!s_http.initialized) return ESP_ERR_INVALID_STATE;
    if (s_http.started)      return ESP_OK;
    httpd_config_t httpd_cfg    = HTTPD_DEFAULT_CONFIG();
    httpd_cfg.server_port       = s_http.config.port;
    httpd_cfg.max_uri_handlers  = s_http.config.max_uri_handlers;
    httpd_cfg.stack_size        = s_http.config.stack_size;
    httpd_cfg.lru_purge_enable  = true;
    esp_err_t err = httpd_start(&s_http.handle, &httpd_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to start httpd: %s", esp_err_to_name(err));
        return err;
    }
    for (size_t i = 0; i < s_http.config.route_count; i++) {
        err = register_route(&s_http.config.routes[i]);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "failed to register route %s: %s",
                     s_http.config.routes[i].uri, esp_err_to_name(err));
        }
    }
    s_http.started = true;
    ESP_LOGI(TAG, "started on port %d (%d routes)",
             s_http.config.port, (int)s_http.config.route_count);
    return ESP_OK;
}
esp_err_t sky_http_server_stop(void)
{
    if (!s_http.started) return ESP_OK;
    if (s_http.handle) {
        httpd_stop(s_http.handle);
        s_http.handle = NULL;
    }
    s_http.started = false;
    ESP_LOGI(TAG, "stopped");
    return ESP_OK;
}
void sky_http_server_deinit(void)
{
    if (!s_http.initialized) return;
    sky_http_server_stop();
    s_http.initialized = false;
    ESP_LOGI(TAG, "deinitialized");
}
esp_err_t sky_http_server_add_route(const sky_http_route_t *route)
{
    if (!route || !route->uri || !route->handler)
        return ESP_ERR_INVALID_ARG;
    if (s_http.started && s_http.handle) {
        return register_route(route);
    }
    ESP_LOGW(TAG, "add_route called before start — use config.routes instead");
    return ESP_ERR_INVALID_STATE;
}
```
## Événements
Ce composant ne publie **aucun événement** dans sa version initiale. C'est une coquille passive :
- Le serveur httpd est géré par le lifecycle `sky_registry`
- Les handlers sont fournis par le firmware produit
Si nécessaire plus tard, on pourra ajouter dans `sky_events.h` :
```c
/* HTTP events  0x0400 – 0x04FF */
#define SKY_EVENT_HTTP_STARTED  0x0400
#define SKY_EVENT_HTTP_STOPPED  0x0401
```
La plage `0x0400` est la première disponible après Power (`0x03xx`).
## Intégration dans main.c
Le composant s'insère **après** `sky_wifi_init` (étape 3) et **avant** `sky_core_start` (étape 10) :
```c
#include "sky_http_server.h"
/* Routes spécifiques Luxia */
static esp_err_t handle_status(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}
static const sky_http_route_t luxia_routes[] = {
    { .uri = "/api/status", .method = HTTP_GET, .handler = handle_status },
};
void app_main(void)
{
    /* ... étapes 1–7 existantes ... */
    /* 8. HTTP Server */
    sky_http_server_config_t http_cfg = SKY_HTTP_SERVER_CONFIG_DEFAULT();
    http_cfg.routes      = luxia_routes;
    http_cfg.route_count = sizeof(luxia_routes) / sizeof(luxia_routes[0]);
    ESP_ERROR_CHECK(sky_http_server_init(&http_cfg));
    /* ... luxia_cmd, event bridges, sky_core_start ... */
}
```
Ajouter à `src/CMakeLists.txt` :
```cmake
REQUIRES sky_core sky_stepper sky_wifi ... sky_http_server
```
## Tâches
- [ ] Créer `components/sky_http_server/` (arborescence complète)
- [ ] Écrire `CMakeLists.txt` avec `REQUIRES sky_core`, `PRIV_REQUIRES esp_http_server`
- [ ] Écrire `Kconfig` (enabled, port, max handlers, stack size)
- [ ] Écrire `include/sky_http_server.h` (route struct, config, API)
- [ ] Écrire `src/sky_http_server.c` (lifecycle, httpd start/stop, register routes)
- [ ] Ajouter `sky_http_server` aux `REQUIRES` de `src/CMakeLists.txt`
- [ ] Ajouter l'init dans `main.c` avec une route `/api/status` de test
- [ ] Valider la compilation avec `pio run`
## Critères de validation
- [ ] `pio run` compile sans erreur ni warning
- [ ] Le serveur HTTP démarre après `sky_core_start()` (log `"started on port 80"`)
- [ ] `curl http://<ip>/api/status` retourne `{"status":"ok"}` quand le device est connecté au Wi-Fi
- [ ] `sky_http_server_stop()` arrête le serveur proprement
- [ ] `add_route()` fonctionne sur un serveur déjà démarré
## Notes techniques
- **Thread safety** : les handlers httpd sont appelés depuis le thread du serveur (`httpd_cfg.stack_size`). Si un handler accède à un état partagé (ex: `s_motor`), il faut soit utiliser un mutex, soit passer par le bus d'événements Sky.
- **RAM** : `esp_http_server` consomme ~3-4 KB par connexion simultanée. Avec `max_open_sockets = 7` (défaut), prévoir ~28 KB de heap.
- **Pas de CORS** : dans cette version initiale, pas de headers CORS. À ajouter si un front-end web est servi depuis un autre origin.
- **Usage prod** : le composant est volontairement générique. Il peut servir en dev (debug endpoints) comme en prod (API locale, configuration web). Le toggle `CONFIG_SKY_HTTP_SERVER_ENABLED` permet de le retirer d'un firmware qui n'en a pas besoin.
